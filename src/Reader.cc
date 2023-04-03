/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "Reader.h"

#include <netcdf>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Variable.h"

namespace {
bool isStringInVector(std::string searchStr, const std::vector<std::string>& strVec) {
  std::cout << "isStringInVector()" << std::endl;
  std::vector<std::string>::const_iterator it = std::find(strVec.begin(), strVec.end(), searchStr);
  if (it != strVec.end())
    return true;
  else
    return false;
}

std::vector<std::string> stringToWords(const std::string& inputStr,
                                       const char separatorChar) {
    std::stringstream stringStream(inputStr);
    std::string word = "";
    std::vector<std::string> wordList;
    while (std::getline(stringStream, word, separatorChar)) {
        wordList.push_back(word);
    }
    return wordList;
}

std::string convertToAtlasDateTimeStr(std::string lfricDateTimeStr) {
  std::vector<std::string> dateTimeSplit = stringToWords(lfricDateTimeStr, ' ');

  return dateTimeSplit[0] + "T" + dateTimeSplit[1] + "Z";
}
}  // anonymous namespace

monio::Reader::Reader(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t mpiRankOwner,
                      const std::string& filePath):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner) {
  std::cout << "Reader::Reader()" << std::endl;
  openFile(filePath);
}

monio::Reader::Reader(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t mpiRankOwner):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner) {
  std::cout << "Reader::Reader()" << std::endl;
}

void monio::Reader::openFile(const std::string& filePath) {
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (filePath.size() != 0) {
      try {
        file_ = std::make_unique<File>(filePath, netCDF::NcFile::read);
      } catch (netCDF::exceptions::NcException& exception) {
        std::string message =
            "Reader::openFile()> An exception occurred while creating File...";
        message.append(exception.what());
        throw std::runtime_error(message);
      }
    }
  }
}

void monio::Reader::readMetadata(Metadata& metadata) {
  std::cout << "Reader::readMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile().readMetadata(metadata);
  }
}

void monio::Reader::readAllData(Metadata& metadata, Data& data) {
  std::cout << "Reader::readAllData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<std::string> varNames = metadata.getVariableNames();
    readSingleData(metadata, data, varNames);
  }
}

void monio::Reader::readFieldDatum(Metadata& metadata,
                                   Data& data,
                                   const std::string& varName,
                                   const size_t timeStep,
                                   const std::string& timeDimName) {
  std::cout << "Reader::readFieldDatum()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = data.getContainer(varName);
    if (dataContainer == nullptr) {
      std::shared_ptr<Variable> variable = metadata.getVariable(varName);
      int dataType = variable->getType();

      std::vector<size_t> startVec;
      std::vector<size_t> countVec;

      startVec.push_back(timeStep);
      countVec.push_back(1);

      size_t varSizeNoTime = 1;
      std::vector<std::pair<std::string, size_t>> dimensions = variable->getDimensionsMap();
      for (auto const& dimPair : dimensions) {
        if (dimPair.first != timeDimName) {
          varSizeNoTime *= dimPair.second;
          startVec.push_back(0);
          countVec.push_back(dimPair.second);
          std::cout << "dimPair.first> " << dimPair.first <<
                              ", dimPair.second> " << dimPair.second << std::endl;
        }
      }
      switch (dataType) {
        case constants::eDataTypes::eDouble: {
          std::shared_ptr<DataContainerDouble> dataContainerDouble =
                                      std::make_shared<DataContainerDouble>(varName);
          getFile().readFieldDatum(varName, varSizeNoTime,
                              startVec, countVec, dataContainerDouble->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
          break;
        }
        case constants::eDataTypes::eFloat: {
          std::shared_ptr<DataContainerFloat> dataContainerFloat =
                                      std::make_shared<DataContainerFloat>(varName);
          getFile().readFieldDatum(varName, varSizeNoTime,
                               startVec, countVec, dataContainerFloat->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
          break;
        }
        case constants::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
                                      std::make_shared<DataContainerInt>(varName);
          getFile().readFieldDatum(varName, varSizeNoTime,
                               startVec, countVec, dataContainerInt->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
          break;
        }
        default:
          throw std::runtime_error("Reader::readFieldData()> Data type not coded for...");
      }
      if (dataContainer != nullptr) {
        data.addContainer(dataContainer);
      } else {
        throw std::runtime_error("Reader::readFieldData()> "
           "An exception occurred while creating data container...");
      }
    } else {
      std::cout << "Reader::readFieldDatum()> DataContainer \""
        << varName << "\" aleady defined." << std::endl;
    }
  }
}

void monio::Reader::readSingleData(Metadata& metadata, Data& data, const std::vector<std::string>& varNames) {
  std::cout << "Reader::readSingleData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    for (const auto& varName : varNames) {
      readSingleDatum(metadata, data, varName);
    }
  }
}

void monio::Reader::readSingleDatum(Metadata& metadata, Data& data, const std::string& varName) {
  std::cout << "Reader::readSingleDatum()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    std::shared_ptr<Variable> variable = metadata.getVariable(varName);
    int dataType = variable->getType();
    switch (dataType) {
      case constants::eDataTypes::eDouble: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                            std::make_shared<DataContainerDouble>(varName);
        getFile().readSingleDatum(varName,
            variable->getTotalSize(), dataContainerDouble->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
        break;
      }
      case constants::eDataTypes::eFloat: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                            std::make_shared<DataContainerFloat>(varName);
        getFile().readSingleDatum(varName,
            variable->getTotalSize(), dataContainerFloat->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
        break;
      }
      case constants::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
                            std::make_shared<DataContainerInt>(varName);
        getFile().readSingleDatum(varName,
            variable->getTotalSize(), dataContainerInt->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
        break;
      }
      default:
        throw std::runtime_error("Reader::readVariable()> Data type not coded for...");
    }
    if (dataContainer != nullptr)
      data.addContainer(dataContainer);
    else
      throw std::runtime_error("Reader::readVariable()> "
          "An exception occurred while creating data container...");
  }
}

monio::File& monio::Reader::getFile() {
  std::cout << "Reader::getFile()" << std::endl;
  if (file_ == nullptr)
    throw std::runtime_error("Reader::getFile()> File has not been initialised...");

  return *file_;
}

std::vector<std::string> monio::Reader::getVarStrAttrs(const Metadata& metadata,
                                                       const std::vector<std::string>& varNames,
                                                       const std::string& attrName) {
  std::cout << "Reader::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varStrAttrs;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    varStrAttrs = metadata.getVarStrAttrs(varNames, attrName);
  }
  return varStrAttrs;
}

std::vector<std::shared_ptr<monio::DataContainerBase>> monio::Reader::getCoordData(Data& data,
                                                   const std::vector<std::string>& coordNames) {
  std::cout << "Reader::getCoordData()" << std::endl;
  if (coordNames.size() == 2) {
    std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
    if (mpiCommunicator_.rank() == mpiRankOwner_) {
      std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                            data.getContainers();
      for (auto& dataPair : dataContainers) {
        if (isStringInVector(dataPair.first, coordNames) == true) {
          std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(dataPair.first);
          coordContainers.push_back(dataContainer);
        }
      }
    }
    return coordContainers;
  } else {
      throw std::runtime_error("Reader::getCoordData()> Incorrect number of coordinate axes...");
  }
}

std::vector<monio::constants::FieldMetadata> monio::Reader::getFieldMetadata(
                                           const Metadata& metadata,
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm) {
  std::cout << "Reader::getFieldMetadata()" << std::endl;
  // No MPI rank check - used to call private functions that broadcast data to all PEs
  std::vector<constants::FieldMetadata> fieldMetadataVec;
  for (auto lfricIt = lfricFieldNames.begin(), atlasIt = atlasFieldNames.begin();
                            lfricIt != lfricFieldNames.end(); ++lfricIt , ++atlasIt) {
    struct constants::FieldMetadata fieldMetadata;
    fieldMetadata.lfricName = *lfricIt;
    fieldMetadata.atlasName = *atlasIt;
    fieldMetadata.numLevels = getVarNumLevels(metadata, fieldMetadata.lfricName, levelsSearchTerm);
    fieldMetadata.dataType = getVarDataType(metadata, fieldMetadata.lfricName);
    fieldMetadata.fieldSize = getSizeOwned(metadata, fieldMetadata.lfricName);
    fieldMetadataVec.push_back(fieldMetadata);
  }
  return fieldMetadataVec;
}

size_t monio::Reader::getSizeOwned(const Metadata& metadata, const std::string& varName) {
  std::cout << "Reader::getSizeOwned()" << std::endl;
  size_t totalSize;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = metadata.getVariable(varName);
    totalSize = variable->getTotalSize();
  }
  mpiCommunicator_.broadcast(totalSize, mpiRankOwner_);
  return totalSize;
}

size_t monio::Reader::getVarNumLevels(const Metadata& metadata,
                                      const std::string& varName,
                                      const std::string& levelsSearchTerm) {
  std::cout << "Reader::getVarNumLevels()" << std::endl;
  size_t numLevels;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = metadata.getVariable(varName);
    numLevels = variable->findDimensionSize(levelsSearchTerm);
  }
  mpiCommunicator_.broadcast(numLevels, mpiRankOwner_);
  return numLevels;
}

int monio::Reader::getVarDataType(const Metadata& metadata, const std::string& varName) {
  std::cout << "Reader::getVarDataType()" << std::endl;
  int dataType;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = metadata.getVariable(varName);
    dataType = variable->getType();
  }
  mpiCommunicator_.broadcast(dataType, mpiRankOwner_);
  return dataType;
}
