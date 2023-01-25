
/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFFile.h"

#include <map>
#include <memory>
#include <stdexcept>

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Logger.h"

#include "NetCDFAttributeBase.h"
#include "NetCDFAttributeInt.h"
#include "NetCDFAttributeString.h"
#include "NetCDFConstants.h"
#include "NetCDFVariable.h"

// Constructors ///////////////////////////////////////////////////////////////////////////////////

lfriclite::NetCDFFile::NetCDFFile(const std::string& filePath,
                                  const netCDF::NcFile::FileMode fileMode):
                                  filePath_(filePath),
                                  fileMode_(fileMode) {
  try {
    oops::Log::debug() << "NetCDFFile::NetCDFFile(): filePath_> " <<  filePath_  <<
                         ", fileMode_> " << fileMode_ << std::endl;
    dataFile_ = std::make_unique<netCDF::NcFile>(filePath_, fileMode_);
  } catch (netCDF::exceptions::NcException& exception) {
    std::string message = "An exception occurred in NetCDFFile> ";
    message.append(exception.what());
    throw std::runtime_error(message);
  }
}

lfriclite::NetCDFFile::~NetCDFFile() {}

// Reading functions //////////////////////////////////////////////////////////////////////////////

void lfriclite::NetCDFFile::readMetadata(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::readMetadata()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    readDimensions(metadata);  // Should be called before readVariables()
    readVariables(metadata);
    readAttributes(metadata);  // Global attributes
    metadata.print();
  } else {
    throw std::runtime_error(
        "NetCDFFile::readMetadata()> Write file accessed for reading...");
  }
}

void lfriclite::NetCDFFile::readMetadata(NetCDFMetadata& metadata,
                                         const std::vector<std::string>& varNames) {
  oops::Log::debug() << "NetCDFFile::readMetadata()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    readDimensions(metadata);  // Should be called before readVariables()
    readVariables(metadata, varNames);
    readAttributes(metadata);  // Global attributes
    metadata.print();
  } else {
    throw std::runtime_error(
        "NetCDFFile::readMetadata()> Write file accessed for reading...");
  }
}

void lfriclite::NetCDFFile::readDimensions(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::readDimensions()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    std::multimap<std::string, netCDF::NcDim> ncDimsMap = getFile()->getDims();
    for (auto const& ncDimPair : ncDimsMap) {
      int value = (ncDimPair.second).getSize();
      metadata.addDimension(ncDimPair.first, value);
    }
  } else {
    throw std::runtime_error(
        "NetCDFFile::readDimensions()> Write file accessed for reading...");
  }
}

void lfriclite::NetCDFFile::readVariables(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    // Potentially process getFile()->getGroups() OR getFile()->getId() here?
    std::multimap<std::string, netCDF::NcVar> nvVarsMap = getFile()->getVars();
    for (auto const& ncVarPair : nvVarsMap) {
      netCDF::NcVar ncVar = ncVarPair.second;
      readVariable(metadata, ncVar);
    }
  } else {
    throw std::runtime_error("NetCDFFile::readVariables()> Write file accessed for reading...");
  }
}

void lfriclite::NetCDFFile::readVariables(NetCDFMetadata& metadata,
                                          const std::vector<std::string>& variableNames) {
  oops::Log::debug() << "NetCDFFile::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    // Potentially process getFile()->getGroups() OR getFile()->getId() here?
    std::multimap<std::string, netCDF::NcVar> nvVarsMap = getFile()->getVars();
    for (auto const& ncVarPair : nvVarsMap) {
      netCDF::NcVar ncVar = ncVarPair.second;
      if (std::find(variableNames.begin(), variableNames.end(),
          ncVar.getName()) != variableNames.end())
      {
        readVariable(metadata, ncVar);
      }
    }
  } else {
    throw std::runtime_error("NetCDFFile::readVariables()> Write file accessed for reading...");
  }
}

void lfriclite::NetCDFFile::readVariable(NetCDFMetadata& metadata, netCDF::NcVar ncVar) {
  netCDF::NcType varType = ncVar.getType();
  std::string varName = ncVar.getName();
  lfriclite::NetCDFVariable* var;
  if (varType == netCDF::NcType::nc_INT)
    var = new NetCDFVariable(varName, lfriclite::ncconsts::eInt);
  else if (varType == netCDF::NcType::nc_FLOAT)
    var = new NetCDFVariable(varName, lfriclite::ncconsts::eFloat);
  else if (varType == netCDF::NcType::nc_DOUBLE)
    var = new NetCDFVariable(varName, lfriclite::ncconsts::eDouble);
  else
    throw std::runtime_error("NetCDFFile::readVariables()> Variable data type " +
                                varType.getName() + " not coded for.");

  std::vector<netCDF::NcDim> ncVarDims = ncVar.getDims();
  int varTotalSize = 1;
  for (auto const& ncVarDim : ncVarDims) {
    std::string varDimName = ncVarDim.getName();
    if (metadata.isDimDefined(varDimName) == false) {
      throw std::runtime_error("NetCDFFile::readVariables()> Variable dimension \"" +
                               varDimName + "\" not defined.");
    }
    std::size_t varDimSize = ncVarDim.getSize();
    var->addDimension(varDimName, varDimSize);
    // Potentially store varDim.getId() OR varDim.isNull() here?
    varTotalSize *= varDimSize;
  }
  var->setTotalSize(varTotalSize);

  std::map<std::string, netCDF::NcVarAtt> ncVarAttrMap = ncVar.getAtts();
  for (auto const& ncVarAttrPair : ncVarAttrMap) {
    netCDF::NcVarAtt ncVarAttr = ncVarAttrPair.second;

    lfriclite::NetCDFAttributeBase* varAttr = nullptr;
    netCDF::NcType ncVarAttrType = ncVarAttr.getType();

    if (ncVarAttrType == netCDF::NcType::nc_CHAR ||
        ncVarAttrType == netCDF::NcType::nc_STRING) {
      std::string strValue;
      ncVarAttr.getValues(strValue);
      varAttr = new lfriclite::NetCDFAttributeString(ncVarAttr.getName(), strValue);
      var->addAttribute(varAttr);
    } else if (ncVarAttrType == netCDF::NcType::nc_INT ||
               ncVarAttrType == netCDF::NcType::nc_SHORT) {
      int intValue;
      ncVarAttr.getValues(&intValue);
      varAttr = new lfriclite::NetCDFAttributeInt(ncVarAttr.getName(), intValue);
      var->addAttribute(varAttr);
    } else {
      throw std::runtime_error("NetCDFFile::readVariables()> Variable attribute data type \""
                                  + ncVarAttrType.getName() + "\" not coded for.");
    }
  }
  metadata.addVariable(var->getName(), var);
}


void lfriclite::NetCDFFile::readAttributes(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::readVariables()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    std::multimap<std::string, netCDF::NcGroupAtt> ncAttrMap = getFile()->getAtts();
    for (auto const& ncAttrPair : ncAttrMap) {
      netCDF::NcGroupAtt ncAttr = ncAttrPair.second;

      lfriclite::NetCDFAttributeBase* globAttr = nullptr;
      netCDF::NcType attrType = ncAttr.getType();
      if (attrType == netCDF::NcType::nc_CHAR || attrType == netCDF::NcType::nc_STRING) {
        std::string strValue;
        ncAttr.getValues(strValue);
        globAttr = new lfriclite::NetCDFAttributeString(ncAttr.getName(), strValue);

      } else if (attrType == netCDF::NcType::nc_INT || attrType == netCDF::NcType::nc_SHORT) {
        int intValue;
        ncAttr.getValues(&intValue);
        globAttr = new lfriclite::NetCDFAttributeInt(ncAttr.getName(), intValue);
      } else {
        throw std::runtime_error("NetCDFFile::readAttributes()> Global attribute data type \""
                                 + attrType.getName() + "\" not coded for.");
      }
      metadata.addGlobalAttr(globAttr->getName(), globAttr);
    }
  } else {
    throw std::runtime_error(
        "NetCDFFile::readAttributes()> Write file accessed for reading...");
  }
}

template<typename T>
void lfriclite::NetCDFFile::readData(const std::string& varName,
                                     const int varSize,
                                     std::vector<T>& dataVec) {
  oops::Log::debug() << "NetCDFFile::readData()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile()->getVar(varName);
    dataVec.resize(varSize, 0);
    var.getVar(dataVec.data());
  } else {
    throw std::runtime_error("NetCDFFile::readData()> Write file accessed for reading...");
  }
}

template void lfriclite::NetCDFFile::readData<double>(const std::string& varName,
                                                      const int varSize,
                                                      std::vector<double>& dataVec);
template void lfriclite::NetCDFFile::readData<float>(const std::string& varName,
                                                     const int varSize,
                                                     std::vector<float>& dataVec);
template void lfriclite::NetCDFFile::readData<int>(const std::string& varName,
                                                   const int varSize,
                                                   std::vector<int>& dataVec);

template<typename T>
void lfriclite::NetCDFFile::readField(const std::string& fieldName,
                                      const int varSize,
                                      const std::vector<size_t>& startVec,
                                      const std::vector<size_t>& countVec,
                                      std::vector<T>& dataVec) {
  oops::Log::debug() << "NetCDFFile::readField()" << std::endl;
  if (fileMode_ == netCDF::NcFile::read) {
    auto var = getFile()->getVar(fieldName);
    dataVec.resize(varSize, 0);
    var.getVar(startVec, countVec, dataVec.data());
  } else {
    throw std::runtime_error("NetCDFFile::readField()> Write file accessed for reading...");
  }
}

template void lfriclite::NetCDFFile::readField<double>(const std::string& varName,
                                                       const int varSize,
                                                       const std::vector<size_t>& startVec,
                                                       const std::vector<size_t>& countVec,
                                                       std::vector<double>& dataVec);
template void lfriclite::NetCDFFile::readField<float>(const std::string& varName,
                                                      const int varSize,
                                                      const std::vector<size_t>& startVec,
                                                      const std::vector<size_t>& countVec,
                                                      std::vector<float>& dataVec);
template void lfriclite::NetCDFFile::readField<int>(const std::string& varName,
                                                    const int varSize,
                                                    const std::vector<size_t>& startVec,
                                                    const std::vector<size_t>& countVec,
                                                    std::vector<int>& dataVec);

// Writing functions //////////////////////////////////////////////////////////////////////////////

void lfriclite::NetCDFFile::writeMetadata(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::writeMetadata()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    writeDimensions(metadata);  // Should be called before readVariables()
    writeVariables(metadata);
    writeAttributes(metadata);  // Global attributes
  } else {
    throw std::runtime_error(
        "NetCDFFile::writeMetadata()> Read file accessed for writing...");
  }
}

void lfriclite::NetCDFFile::writeDimensions(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::writeDimensions()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    std::map<std::string, int>& dimMap = metadata.getDimensionsMap();
    for (auto const& dimPair : dimMap) {
      getFile()->addDim(dimPair.first, dimPair.second);
    }
  } else {
    throw std::runtime_error("NetCDFFile::writeDimensions()> Read file accessed for writing...");
  }
}

void lfriclite::NetCDFFile::writeVariables(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::writeVariables()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    std::map<std::string, NetCDFVariable*>& varsMap = metadata.getVariablesMap();
    for (auto const& varPair : varsMap) {
      NetCDFVariable* var = varPair.second;

      netCDF::NcVar ncVar = getFile()->addVar(var->getName(),
                            lfriclite::ncconsts::kDataTypeNames[var->getType()],
                            var->getDimensionKeys());

      std::map<std::string, NetCDFAttributeBase*>& varAttrsMap = var->getAttributes();
      for (const auto& varAttrPair : varAttrsMap) {
        NetCDFAttributeBase* varAttr = varAttrPair.second;
        int varAttrType = varAttr->getType();
        if (varAttrType == lfriclite::ncconsts::eString) {
          NetCDFAttributeString* varAttrStr = static_cast<NetCDFAttributeString*>(varAttr);
          ncVar.putAtt(varAttrStr->getName(), varAttrStr->getValue());
        } else if (varAttrType == lfriclite::ncconsts::eInt) {
          NetCDFAttributeInt* varAttrInt = static_cast<NetCDFAttributeInt*>(varAttr);
          ncVar.putAtt(varAttrInt->getName(), netCDF::NcType::nc_INT, varAttrInt->getValue());
        } else {
          throw std::runtime_error("NetCDFFile::writeVariables()> "
              "Variable attribute data type not coded for...");
        }
      }
    }
  } else {
    throw std::runtime_error("NetCDFFile::writeVariables()> Read file accessed for writing...");
  }
}

void lfriclite::NetCDFFile::writeAttributes(NetCDFMetadata& metadata) {
  oops::Log::debug() << "NetCDFFile::writeAttributes()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    std::map<std::string, NetCDFAttributeBase*>& globalAttrMap = metadata.getGlobalAttrsMap();
    for (auto const& globalAttrPair : globalAttrMap) {
      NetCDFAttributeBase* globAttr = globalAttrPair.second;

      if (globAttr->getType() == lfriclite::ncconsts::eString) {
        NetCDFAttributeString* globAttrStr = static_cast<NetCDFAttributeString*>(globAttr);
        std::string globAttrName = globAttrStr->getName();
        getFile()->putAtt(globAttrName, globAttrStr->getValue());
      } else if (globAttr->getType() == lfriclite::ncconsts::eInt) {
        NetCDFAttributeInt* globAttrInt = static_cast<NetCDFAttributeInt*>(globAttr);
        getFile()->putAtt(globAttrInt->getName(), netCDF::NcType::nc_INT, globAttrInt->getValue());
      } else {
        throw std::runtime_error("NetCDFFile::writeVariables()> "
            "Variable attribute data type not coded for...");
      }
    }
  } else {
    throw std::runtime_error("NetCDFFile::writeAttributes()> Read file accessed for writing...");
  }
}

template<typename T>
void lfriclite::NetCDFFile::writeData(const std::string &varName, const std::vector<T>& dataVec) {
  oops::Log::debug() << "NetCDFFile::writeData()" << std::endl;
  if (fileMode_ != netCDF::NcFile::read) {
    auto var = getFile()->getVar(varName);
    var.putVar(dataVec.data());
  } else {
    throw std::runtime_error("NetCDFFile::writeData()> Read file accessed for writing...");
  }
}

template void lfriclite::NetCDFFile::writeData<double>(const std::string& varName,
                                                       const std::vector<double>& dataVec);
template void lfriclite::NetCDFFile::writeData<float>(const std::string& varName,
                                                      const std::vector<float>& dataVec);
template void lfriclite::NetCDFFile::writeData<int>(const std::string& varName,
                                                    const std::vector<int>& dataVec);

// Encapsulation functions ////////////////////////////////////////////////////////////////////////

const std::string& lfriclite::NetCDFFile::getPath() {
  return filePath_;
}

void lfriclite::NetCDFFile::setPath(const std::string filePath) {
  filePath_ = filePath;
}

const netCDF::NcFile::FileMode& lfriclite::NetCDFFile::getFileMode() {
  return fileMode_;
}

void lfriclite::NetCDFFile::setFileMode(const netCDF::NcFile::FileMode fileMode) {
  fileMode_ = fileMode;
}

const bool lfriclite::NetCDFFile::isRead() {
  return fileMode_ == netCDF::NcFile::read;
}

const bool lfriclite::NetCDFFile::isWrite() {
  return fileMode_ != netCDF::NcFile::read;
}

netCDF::NcFile* lfriclite::NetCDFFile::getFile() {
  if (dataFile_ == nullptr)
    throw std::runtime_error("NetCDFFile::getFile()> Data file has not been initialised...");

  return dataFile_.get();
}
