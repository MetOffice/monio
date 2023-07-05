
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
#include "AttributeInt.h"
#include "AttributeString.h"
#include "Constants.h"
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
    throw std::runtime_error(message);
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
    throw std::runtime_error(
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
    throw std::runtime_error(
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
    throw std::runtime_error(
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
    throw std::runtime_error("File::readVariables()> Write file accessed for reading...");
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
    throw std::runtime_error("File::readVariables()> Write file accessed for reading...");
  }
}

void monio::File::readVariable(Metadata& metadata, netCDF::NcVar ncVar) {
  netCDF::NcType varType = ncVar.getType();
  std::string varName = ncVar.getName();
  std::shared_ptr<monio::Variable> var = nullptr;
  if (varType == netCDF::NcType::nc_INT)
    var = std::make_shared<Variable>(varName, consts::eInt);
  else if (varType == netCDF::NcType::nc_FLOAT)
    var = std::make_shared<Variable>(varName, consts::eFloat);
  else if (varType == netCDF::NcType::nc_DOUBLE)
    var = std::make_shared<Variable>(varName, consts::eDouble);
  else
    throw std::runtime_error("File::readVariable()> Variable data type " +
                                varType.getName() + " not coded for.");

  std::vector<netCDF::NcDim> ncVarDims = ncVar.getDims();
  for (auto const& ncVarDim : ncVarDims) {
    std::string varDimName = ncVarDim.getName();
    if (metadata.isDimDefined(varDimName) == false) {
      throw std::runtime_error("File::readVariable()> Variable dimension \"" +
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

    if (ncVarAttrType == netCDF::NcType::nc_CHAR ||
        ncVarAttrType == netCDF::NcType::nc_STRING) {
      std::string strValue;
      ncVarAttr.getValues(strValue);
      varAttr = std::make_shared<AttributeString>(ncVarAttr.getName(), strValue);
      var->addAttribute(varAttr);
    } else if (ncVarAttrType == netCDF::NcType::nc_INT ||
               ncVarAttrType == netCDF::NcType::nc_SHORT) {
      int intValue;
      ncVarAttr.getValues(&intValue);
      varAttr = std::make_shared<AttributeInt>(ncVarAttr.getName(), intValue);
      var->addAttribute(varAttr);
    } else {
      throw std::runtime_error("File::readVariable()> Variable attribute data type \""
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
      if (attrType == netCDF::NcType::nc_CHAR || attrType == netCDF::NcType::nc_STRING) {
        std::string strValue;
        ncAttr.getValues(strValue);
        globAttr = std::make_shared<AttributeString>(ncAttr.getName(), strValue);

      } else if (attrType == netCDF::NcType::nc_INT || attrType == netCDF::NcType::nc_SHORT) {
        int intValue;
        ncAttr.getValues(&intValue);
        globAttr = std::make_shared<AttributeInt>(ncAttr.getName(), intValue);
      } else {
        throw std::runtime_error("File::readAttributes()> Global attribute data type \""
                                 + attrType.getName() + "\" not coded for.");
      }
      metadata.addGlobalAttr(globAttr->getName(), globAttr);
    }
  } else {
    throw std::runtime_error(
        "File::readAttributes()> Write file accessed for reading...");
  }
}

template<typename T>
void monio::File::readSingleDatum(const std::string& varName,
                                  const int varSize,
                                  std::vector<T>& dataVec) {
  oops::Log::debug() << "File::readData()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile().getVar(varName);
    dataVec.resize(varSize, 0);
    var.getVar(dataVec.data());
  } else {
    throw std::runtime_error("File::readSingleDatum()> Write file accessed for reading...");
  }
}

template void monio::File::readSingleDatum<double>(const std::string& varName,
                                                   const int varSize,
                                                   std::vector<double>& dataVec);
template void monio::File::readSingleDatum<float>(const std::string& varName,
                                                  const int varSize,
                                                  std::vector<float>& dataVec);
template void monio::File::readSingleDatum<int>(const std::string& varName,
                                                const int varSize,
                                                std::vector<int>& dataVec);

template<typename T>
void monio::File::readFieldDatum(const std::string& fieldName,
                                 const int varSize,
                                 const std::vector<size_t>& startVec,
                                 const std::vector<size_t>& countVec,
                                 std::vector<T>& dataVec) {
  oops::Log::debug() << "File::readFieldDatum()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile().getVar(fieldName);
    dataVec.resize(varSize, 0);
    var.getVar(startVec, countVec, dataVec.data());
  } else {
    throw std::runtime_error("File::readFieldDatum()> Write file accessed for reading...");
  }
}

template void monio::File::readFieldDatum<double>(const std::string& varName,
                                                  const int varSize,
                                                  const std::vector<size_t>& startVec,
                                                  const std::vector<size_t>& countVec,
                                                  std::vector<double>& dataVec);
template void monio::File::readFieldDatum<float>(const std::string& varName,
                                                 const int varSize,
                                                 const std::vector<size_t>& startVec,
                                                 const std::vector<size_t>& countVec,
                                                 std::vector<float>& dataVec);
template void monio::File::readFieldDatum<int>(const std::string& varName,
                                               const int varSize,
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
    throw std::runtime_error(
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
    throw std::runtime_error("File::writeDimensions()> Read file accessed for writing...");
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
        int varAttrType = varAttr->getType();
        if (varAttrType == consts::eString) {
          std::shared_ptr<AttributeString> varAttrStr =
                        std::dynamic_pointer_cast<AttributeString>(varAttr);
          ncVar.putAtt(varAttrStr->getName(), varAttrStr->getValue());
        } else if (varAttrType == consts::eInt) {
          std::shared_ptr<AttributeInt> varAttrInt =
                        std::dynamic_pointer_cast<AttributeInt>(varAttr);
          ncVar.putAtt(varAttrInt->getName(), netCDF::NcType::nc_INT, varAttrInt->getValue());
        } else {
          throw std::runtime_error("File::writeVariables()> "
              "Variable attribute data type not coded for...");
        }
      }
    }
  } else {
    throw std::runtime_error("File::writeVariables()> Read file accessed for writing...");
  }
}

void monio::File::writeAttributes(const Metadata& metadata) {
  oops::Log::debug() << "File::writeAttributes()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    const std::map<std::string, std::shared_ptr<AttributeBase>>& globalAttrMap =
                                metadata.getGlobalAttrsMap();
    for (auto const& globalAttrPair : globalAttrMap) {
      std::shared_ptr<AttributeBase> globAttr = globalAttrPair.second;

      if (globAttr->getType() == consts::eString) {
        std::shared_ptr<AttributeString> globAttrStr =
                                         std::static_pointer_cast<AttributeString>(globAttr);
        std::string globAttrName = globAttrStr->getName();
        getFile().putAtt(globAttrName, globAttrStr->getValue());
      } else if (globAttr->getType() == consts::eInt) {
        std::shared_ptr<AttributeInt> globAttrInt =
                                      std::static_pointer_cast<AttributeInt>(globAttr);
        getFile().putAtt(globAttrInt->getName(), netCDF::NcType::nc_INT, globAttrInt->getValue());
      } else {
        throw std::runtime_error("File::writeVariables()> "
            "Variable attribute data type not coded for...");
      }
    }
  } else {
    throw std::runtime_error("File::writeAttributes()> Read file accessed for writing...");
  }
}

template<typename T>
void monio::File::writeSingleDatum(const std::string &varName, const std::vector<T>& dataVec) {
  oops::Log::debug() << "File::writeSingleDatum()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    auto var = getFile().getVar(varName);
    var.putVar(dataVec.data());
  } else {
    throw std::runtime_error("File::writeSingleDatum()> Read file accessed for writing...");
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
    throw std::runtime_error("File::getFile()> Data file has not been initialised...");

  return *dataFile_;
}
