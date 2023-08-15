
/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "File.h"

#include <map>
#include <memory>
#include <stdexcept>

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Logger.h"

#include "AttributeBase.h"
#include "AttributeDouble.h"
#include "AttributeInt.h"
#include "AttributeString.h"
#include "Constants.h"
#include "Utils.h"
#include "Variable.h"

// De/Constructors ////////////////////////////////////////////////////////////////////////////////

monio::File::File(const std::string& filePath,
                  const netCDF::NcFile::FileMode fileMode):
                  filePath_(filePath),
                  fileMode_(fileMode) {
  try {
    oops::Log::debug() << "File::File(): filePath_> " <<  filePath_  <<
                         ", fileMode_> " << fileMode_ << std::endl;
    dataFile_ = std::make_unique<netCDF::NcFile>(filePath_, fileMode_);
  } catch (netCDF::exceptions::NcException& exception) {
    std::string message = "An exception occurred in File> ";
    message.append(exception.what());
    utils::throwException(message);
  }
}

monio::File::~File() {
  std::cout << "File::~File() ";
  if (fileMode_ == netCDF::NcFile::read) {
    std::cout << "read" << std::endl;
  } else {
    std::cout << "write" << std::endl;
  }
  dataFile_->close();
}

// Reading functions //////////////////////////////////////////////////////////////////////////////

void monio::File::readMetadata(Metadata& metadata) {
  oops::Log::debug() << "File::readMetadata()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    readDimensions(metadata);  // Should be called before readVariables()
    readVariables(metadata);
    readAttributes(metadata);  // Global attributes
    metadata.print();
  } else {
    utils::throwException(
        "File::readMetadata()> Write file accessed for reading...");
  }
}

void monio::File::readMetadata(Metadata& metadata,
                               const std::vector<std::string>& varNames) {
  oops::Log::debug() << "File::readMetadata()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    readDimensions(metadata);  // Should be called before readVariables()
    readVariables(metadata, varNames);
    readAttributes(metadata);  // Global attributes
    metadata.print();
  } else {
    utils::throwException(
        "File::readMetadata()> Write file accessed for reading...");
  }
}

void monio::File::readDimensions(Metadata& metadata) {
  oops::Log::debug() << "File::readDimensions()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    std::multimap<std::string, netCDF::NcDim> ncDimsMap = getFile().getDims();
    for (auto const& ncDimPair : ncDimsMap) {
      int value = (ncDimPair.second).getSize();
      metadata.addDimension(ncDimPair.first, value);
    }
  } else {
    utils::throwException(
        "File::readDimensions()> Write file accessed for reading...");
  }
}

void monio::File::readVariables(Metadata& metadata) {
  oops::Log::debug() << "File::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    // Potentially process getFile().getGroups() OR getFile().getId() here?
    std::multimap<std::string, netCDF::NcVar> nvVarsMap = getFile().getVars();
    for (auto const& ncVarPair : nvVarsMap) {
      netCDF::NcVar ncVar = ncVarPair.second;
      readVariable(metadata, ncVar);
    }
  } else {
    utils::throwException("File::readVariables()> Write file accessed for reading...");
  }
}

void monio::File::readVariables(Metadata& metadata,
                                const std::vector<std::string>& variableNames) {
  oops::Log::debug() << "File::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    // Potentially process getFile().getGroups() OR getFile().getId() here?
    std::multimap<std::string, netCDF::NcVar> nvVarsMap = getFile().getVars();
    for (auto const& ncVarPair : nvVarsMap) {
      netCDF::NcVar ncVar = ncVarPair.second;
      if (std::find(variableNames.begin(), variableNames.end(),
          ncVar.getName()) != variableNames.end()) {
        readVariable(metadata, ncVar);
      }
    }
  } else {
    utils::throwException("File::readVariables()> Write file accessed for reading...");
  }
}

void monio::File::readVariable(Metadata& metadata, netCDF::NcVar ncVar) {
  netCDF::NcType varType = ncVar.getType();
  std::string varName = ncVar.getName();
  std::shared_ptr<monio::Variable> var = nullptr;

  switch (varType.getId()) {
    case netCDF::NcType::nc_DOUBLE: {
      var = std::make_shared<Variable>(varName, consts::eDouble);
      break;
    }
    case netCDF::NcType::nc_FLOAT: {
      var = std::make_shared<Variable>(varName, consts::eFloat);
      break;
    }
    case netCDF::NcType::nc_INT: {
      var = std::make_shared<Variable>(varName, consts::eInt);
      break;
    }
    default:
      utils::throwException("File::readVariable()> Variable data type " +
                                  varType.getName() + " not coded for.");
  }
  std::vector<netCDF::NcDim> ncVarDims = ncVar.getDims();
  for (auto const& ncVarDim : ncVarDims) {
    std::string varDimName = ncVarDim.getName();
    if (metadata.isDimDefined(varDimName) == false) {
      utils::throwException("File::readVariable()> Variable dimension \"" +
                               varDimName + "\" not defined.");
    }
    std::size_t varDimSize = ncVarDim.getSize();
    var->addDimension(varDimName, varDimSize);
    // Potentially store varDim.getId() OR varDim.isNull() here?
  }

  std::map<std::string, netCDF::NcVarAtt> ncVarAttrMap = ncVar.getAtts();
  for (auto const& ncVarAttrPair : ncVarAttrMap) {
    netCDF::NcVarAtt ncVarAttr = ncVarAttrPair.second;

    std::shared_ptr<AttributeBase> varAttr = nullptr;
    netCDF::NcType ncVarAttrType = ncVarAttr.getType();

    switch (ncVarAttrType.getId()) {
      case netCDF::NcType::nc_CHAR:
      case netCDF::NcType::nc_STRING: {
        std::string strValue;
        ncVarAttr.getValues(strValue);
        varAttr = std::make_shared<AttributeString>(ncVarAttr.getName(), strValue);
        var->addAttribute(varAttr);
        break;
      }
      case netCDF::NcType::nc_INT:
      case netCDF::NcType::nc_SHORT: {
        int intValue;
        ncVarAttr.getValues(&intValue);
        varAttr = std::make_shared<AttributeInt>(ncVarAttr.getName(), intValue);
        var->addAttribute(varAttr);
        break;
      }
      case netCDF::NcType::nc_FLOAT:
      case netCDF::NcType::nc_DOUBLE: {
        double dblValue;
        ncVarAttr.getValues(&dblValue);
        varAttr = std::make_shared<AttributeDouble>(ncVarAttr.getName(), dblValue);
        var->addAttribute(varAttr);
        break;
      }
      default:
        utils::throwException("File::readVariable()> Variable attribute data type \""
                                    + ncVarAttrType.getName() + "\" not coded for.");
    }
  }
  metadata.addVariable(var->getName(), var);
}

void monio::File::readAttributes(Metadata& metadata) {
  oops::Log::debug() << "File::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    std::multimap<std::string, netCDF::NcGroupAtt> ncAttrMap = getFile().getAtts();
    for (auto const& ncAttrPair : ncAttrMap) {
      netCDF::NcGroupAtt ncAttr = ncAttrPair.second;

      std::shared_ptr<AttributeBase> globAttr = nullptr;
      netCDF::NcType attrType = ncAttr.getType();

      switch (attrType.getId()) {
        case netCDF::NcType::nc_CHAR:
        case netCDF::NcType::nc_STRING: {
          std::string strValue;
          ncAttr.getValues(strValue);
          globAttr = std::make_shared<AttributeString>(ncAttr.getName(), strValue);
          break;
        }
        case netCDF::NcType::nc_INT:
        case netCDF::NcType::nc_SHORT: {
          int intValue;
          ncAttr.getValues(&intValue);
          globAttr = std::make_shared<AttributeInt>(ncAttr.getName(), intValue);
          break;
        }
        case netCDF::NcType::nc_FLOAT:
        case netCDF::NcType::nc_DOUBLE: {
          double dblValue;
          ncAttr.getValues(&dblValue);
          globAttr = std::make_shared<AttributeDouble>(ncAttr.getName(), dblValue);
          break;
        }
        default:
          utils::throwException("File::readAttributes()> Global attribute data type \""
                                   + attrType.getName() + "\" not coded for.");
      }
      metadata.addGlobalAttr(globAttr->getName(), globAttr);
    }
  } else {
    utils::throwException(
        "File::readAttributes()> Write file accessed for reading...");
  }
}

template<typename T>
void monio::File::readSingleDatum(const std::string& varName,
                                  std::vector<T>& dataVec) {
  oops::Log::debug() << "File::readData()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile().getVar(varName);
    var.getVar(dataVec.data());
  } else {
    utils::throwException("File::readSingleDatum()> Write file accessed for reading...");
  }
}

template void monio::File::readSingleDatum<double>(const std::string& varName,
                                                   std::vector<double>& dataVec);
template void monio::File::readSingleDatum<float>(const std::string& varName,
                                                  std::vector<float>& dataVec);
template void monio::File::readSingleDatum<int>(const std::string& varName,
                                                std::vector<int>& dataVec);

template<typename T>
void monio::File::readFieldDatum(const std::string& fieldName,
                                 const std::vector<size_t>& startVec,
                                 const std::vector<size_t>& countVec,
                                 std::vector<T>& dataVec) {
  oops::Log::debug() << "File::readFieldDatum()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile().getVar(fieldName);
    var.getVar(startVec, countVec, dataVec.data());
  } else {
    utils::throwException("File::readFieldDatum()> Write file accessed for reading...");
  }
}

template void monio::File::readFieldDatum<double>(const std::string& varName,
                                                  const std::vector<size_t>& startVec,
                                                  const std::vector<size_t>& countVec,
                                                  std::vector<double>& dataVec);
template void monio::File::readFieldDatum<float>(const std::string& varName,
                                                 const std::vector<size_t>& startVec,
                                                 const std::vector<size_t>& countVec,
                                                 std::vector<float>& dataVec);
template void monio::File::readFieldDatum<int>(const std::string& varName,
                                               const std::vector<size_t>& startVec,
                                               const std::vector<size_t>& countVec,
                                               std::vector<int>& dataVec);

// Writing functions //////////////////////////////////////////////////////////////////////////////

void monio::File::writeMetadata(const Metadata& metadata) {
  oops::Log::debug() << "File::writeMetadata()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    writeDimensions(metadata);  // Should be called before readVariables()
    writeVariables(metadata);
    writeAttributes(metadata);  // Global attributes
  } else {
    utils::throwException(
        "File::writeMetadata()> Read file accessed for writing...");
  }
}

void monio::File::writeDimensions(const Metadata& metadata) {
  oops::Log::debug() << "File::writeDimensions()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    const std::map<std::string, int>& dimMap = metadata.getDimensionsMap();
    for (auto const& dimPair : dimMap) {
      getFile().addDim(dimPair.first, dimPair.second);
    }
  } else {
    utils::throwException("File::writeDimensions()> Read file accessed for writing...");
  }
}

void monio::File::writeVariables(const Metadata& metadata) {
  oops::Log::debug() << "File::writeVariables()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    const std::map<std::string, std::shared_ptr<Variable>>& varsMap = metadata.getVariablesMap();
    for (auto const& varPair : varsMap) {
      std::shared_ptr<Variable> var = varPair.second;
      netCDF::NcVar ncVar = getFile().addVar(var->getName(),
                            std::string(consts::kDataTypeNames[var->getType()]),
                            var->getDimensionNames());

      std::map<std::string, std::shared_ptr<AttributeBase>>& varAttrsMap = var->getAttributes();
      for (const auto& varAttrPair : varAttrsMap) {
        std::shared_ptr<AttributeBase> varAttr = varAttrPair.second;

        switch (varAttr->getType()) {
          case consts::eDataTypes::eDouble: {
            std::shared_ptr<AttributeDouble> varAttrDbl =
                          std::dynamic_pointer_cast<AttributeDouble>(varAttr);
            ncVar.putAtt(varAttrDbl->getName(), netCDF::NcType::nc_DOUBLE, varAttrDbl->getValue());
            break;
          }
          case consts::eDataTypes::eInt: {
            std::shared_ptr<AttributeInt> varAttrInt =
                          std::dynamic_pointer_cast<AttributeInt>(varAttr);
            ncVar.putAtt(varAttrInt->getName(), netCDF::NcType::nc_INT, varAttrInt->getValue());
            break;
          }
          case consts::eDataTypes::eString: {
            std::shared_ptr<AttributeString> varAttrStr =
                          std::dynamic_pointer_cast<AttributeString>(varAttr);
            ncVar.putAtt(varAttrStr->getName(), varAttrStr->getValue());
            break;
          }
          default:
            utils::throwException("File::writeVariables()> "
                "Variable attribute data type not coded for...");
        }
      }
    }
  } else {
    utils::throwException("File::writeVariables()> Read file accessed for writing...");
  }
}

void monio::File::writeAttributes(const Metadata& metadata) {
  oops::Log::debug() << "File::writeAttributes()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    const std::map<std::string, std::shared_ptr<AttributeBase>>& globalAttrMap =
                                metadata.getGlobalAttrsMap();
    for (auto const& globalAttrPair : globalAttrMap) {
      std::shared_ptr<AttributeBase> globAttr = globalAttrPair.second;
      switch (globAttr->getType()) {
        case consts::eDataTypes::eDouble: {
          std::shared_ptr<AttributeDouble> globAttrDbl =
                                           std::static_pointer_cast<AttributeDouble>(globAttr);
          std::string globAttrName = globAttrDbl->getName();
          getFile().putAtt(globAttrName, netCDF::NcType::nc_DOUBLE, globAttrDbl->getValue());
          break;
        }
        case consts::eDataTypes::eInt: {
          std::shared_ptr<AttributeInt> globAttrInt =
                                        std::static_pointer_cast<AttributeInt>(globAttr);
          getFile().putAtt(globAttrInt->getName(), netCDF::NcType::nc_INT, globAttrInt->getValue());
          break;
        }
        case consts::eDataTypes::eString: {
          std::shared_ptr<AttributeString> globAttrStr =
                                           std::static_pointer_cast<AttributeString>(globAttr);
          std::string globAttrName = globAttrStr->getName();
          getFile().putAtt(globAttrName, globAttrStr->getValue());
          break;
        }
        default:
          utils::throwException("File::writeAttributes()> "
              "Variable attribute data type not coded for...");
      }
    }
  } else {
    utils::throwException("File::writeAttributes()> Read file accessed for writing...");
  }
}

template<typename T>
void monio::File::writeSingleDatum(const std::string &varName, const std::vector<T>& dataVec) {
  oops::Log::debug() << "File::writeSingleDatum()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    auto var = getFile().getVar(varName);
    var.putVar(dataVec.data());
  } else {
    utils::throwException("File::writeSingleDatum()> Read file accessed for writing...");
  }
}

template void monio::File::writeSingleDatum<double>(const std::string& varName,
                                                    const std::vector<double>& dataVec);
template void monio::File::writeSingleDatum<float>(const std::string& varName,
                                                   const std::vector<float>& dataVec);
template void monio::File::writeSingleDatum<int>(const std::string& varName,
                                                 const std::vector<int>& dataVec);

// Encapsulation functions ////////////////////////////////////////////////////////////////////////

std::string& monio::File::getPath() {
  return filePath_;
}

void monio::File::setPath(const std::string filePath) {
  filePath_ = filePath;
}

netCDF::NcFile::FileMode& monio::File::getFileMode() {
  return fileMode_;
}

void monio::File::setFileMode(const netCDF::NcFile::FileMode fileMode) {
  fileMode_ = fileMode;
}

bool monio::File::isRead() {
  return fileMode_ == netCDF::NcFile::read;
}

bool monio::File::isWrite() {
  return fileMode_ != netCDF::NcFile::read;
}

netCDF::NcFile& monio::File::getFile() {
  if (dataFile_ == nullptr)
    utils::throwException("File::getFile()> Data file has not been initialised...");

  return *dataFile_;
}
