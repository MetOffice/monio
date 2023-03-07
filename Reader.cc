/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/Reader.h"

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

#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

namespace {
bool isStringInVector(std::string searchStr, const std::vector<std::string>& strVec) {
  oops::Log::debug() << "isStringInVector()" << std::endl;
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
                      const FileData& fileData):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Reader::Reader()" << std::endl;
  openFile(fileData);
}

monio::Reader::Reader(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t mpiRankOwner):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Reader::Reader()" << std::endl;
}

void monio::Reader::openFile(const FileData& fileData) {
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    try {
      file_ = std::make_shared<File>(fileData.getFilePath(), netCDF::NcFile::read);
    } catch (netCDF::exceptions::NcException& exception) {
      std::string message =
          "Reader::openFile()> An exception occurred while creating File...";
      message.append(exception.what());
      throw std::runtime_error(message);
    }
  }
}

void monio::Reader::readMetadata(FileData& fileData) {
  oops::Log::debug() << "Reader::readMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile()->readMetadata(fileData.getMetadata());
  }
}

void monio::Reader::readAllData(FileData& fileData) {
  oops::Log::debug() << "Reader::readAllData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<std::string> varNames = fileData.getMetadata().getVariableNames();
    readSingleData(fileData, varNames);
  }
}

void monio::Reader::createDateTimes(FileData& fileData,
                                    const std::string& timeVarName,
                                    const std::string& timeOriginName) {
  oops::Log::debug() << "Reader::createDateTimes()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getDateTimes().size() != 0)
      throw std::runtime_error("Reader::createDateTimes()> "
                               "Date times already initialised...");

    std::shared_ptr<Variable> timeVar = fileData.getMetadata().getVariable(timeVarName);
    std::shared_ptr<DataContainerBase> timeDataBase = fileData.getData().getContainer(timeVarName);
    if (timeDataBase->getType() != constants::eDouble)
      throw std::runtime_error("Reader::createDateTimes()> "
                               "Time data not stored as double...");

    std::shared_ptr<DataContainerDouble> timeData =
                std::static_pointer_cast<DataContainerDouble>(timeDataBase);

    std::string timeOrigin = timeVar->getStrAttr(timeOriginName);
    std::string timeAtlasOrigin = convertToAtlasDateTimeStr(timeOrigin);

    util::DateTime originDateTime(timeAtlasOrigin);

    oops::Log::debug() << "timeVar->getSize()> " << timeVar->getTotalSize() << std::endl;
    std::vector<util::DateTime> dateTimes(timeVar->getTotalSize());
    for (std::size_t index = 0; index < timeVar->getTotalSize(); ++index) {
      util::Duration duration(static_cast<uint64_t>(std::round(timeData->getDatum(index))));
      util::DateTime dateTime = originDateTime + duration;
      dateTimes[index] = dateTime;
      oops::Log::debug() << "index> " << index << ", data> " << timeData->getDatum(index) <<
                           ", dateTime> " << dateTime << std::endl;
    }
    fileData.setDateTimes(std::move(dateTimes));
  }
}

void monio::Reader::readFieldData(FileData& fileData,
                                  const std::vector<std::string>& varNames,
                                  const std::string& dateString,
                                  const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readFieldData()" << std::endl;
  util::DateTime dateToRead(dateString);
  readFieldData(fileData, varNames, dateToRead, timeDimName);
}

void monio::Reader::readFieldData(FileData& fileData,
                                  const std::vector<std::string>& varNames,
                                  const util::DateTime& dateToRead,
                                  const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readFieldData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    size_t timeStep = findTimeStep(fileData, dateToRead);
    for (auto& varName : varNames) {
      readFieldDatum(fileData, varName, timeStep, timeDimName);
    }
  }
}

void monio::Reader::readFieldDatum(FileData& fileData,
                                   const std::string& varName,
                                   const size_t timeStep,
                                   const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readFieldDatum()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
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
        oops::Log::debug() << "dimPair.first> " << dimPair.first <<
                            ", dimPair.second> " << dimPair.second << std::endl;
      }
    }
    switch (dataType) {
      case constants::eDataTypes::eDouble: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                                    std::make_shared<DataContainerDouble>(varName);
        getFile()->readFieldDatum(varName, varSizeNoTime,
                            startVec, countVec, dataContainerDouble->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
        break;
      }
      case constants::eDataTypes::eFloat: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                                    std::make_shared<DataContainerFloat>(varName);
        getFile()->readFieldDatum(varName, varSizeNoTime,
                             startVec, countVec, dataContainerFloat->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
        break;
      }
      case constants::eDataTypes::eInt: {
      std::shared_ptr<DataContainerInt> dataContainerInt =
                                    std::make_shared<DataContainerInt>(varName);
        getFile()->readFieldDatum(varName, varSizeNoTime,
                             startVec, countVec, dataContainerInt->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
        break;
      }
      default:
        throw std::runtime_error("Reader::readFieldData()> Data type not coded for...");
    }
    if (dataContainer != nullptr)
      fileData.getData().addContainer(dataContainer);
    else
      throw std::runtime_error("Reader::readFieldData()> "
          "An exception occurred while creating data container...");
  }
}

void monio::Reader::readSingleData(FileData& fileData, const std::vector<std::string>& varNames) {
  oops::Log::debug() << "Reader::readSingleData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    for (const auto& varName : varNames) {
      readSingleDatum(fileData, varName);
    }
  }
}

void monio::Reader::readSingleDatum(FileData& fileData, const std::string& varName) {
  oops::Log::debug() << "Reader::readSingleDatum()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
    int dataType = variable->getType();
    switch (dataType) {
      case constants::eDataTypes::eDouble: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                            std::make_shared<DataContainerDouble>(varName);
        getFile()->readSingleDatum(varName,
            variable->getTotalSize(), dataContainerDouble->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
        break;
      }
      case constants::eDataTypes::eFloat: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                            std::make_shared<DataContainerFloat>(varName);
        getFile()->readSingleDatum(varName,
            variable->getTotalSize(), dataContainerFloat->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
        break;
      }
      case constants::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
                            std::make_shared<DataContainerInt>(varName);
        getFile()->readSingleDatum(varName,
            variable->getTotalSize(), dataContainerInt->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
        break;
      }
      default:
        throw std::runtime_error("Reader::readVariable()> Data type not coded for...");
    }

    if (dataContainer != nullptr)
      fileData.getData().addContainer(dataContainer);
    else
      throw std::runtime_error("Reader::readVariable()> "
          "An exception occurred while creating data container...");
  }
}

std::shared_ptr<monio::File> monio::Reader::getFile() {
  oops::Log::debug() << "Reader::getFile()" << std::endl;
  if (file_ == nullptr)
    throw std::runtime_error("Reader::getFile()> File has not been initialised...");

  return file_;
}

std::vector<std::string> monio::Reader::getVarStrAttrs(const FileData& fileData,
                                                       const std::vector<std::string>& varNames,
                                                       const std::string& attrName) {
  oops::Log::debug() << "Reader::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varStrAttrs;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    varStrAttrs = fileData.getMetadata().getVarStrAttrs(varNames, attrName);
  }
  return varStrAttrs;
}

std::vector<std::shared_ptr<monio::DataContainerBase>> monio::Reader::getCoordData(
                                                       FileData& fileData,
                                                   const std::vector<std::string>& coordNames) {
  oops::Log::debug() << "Reader::getCoordData()" << std::endl;
  if (coordNames.size() == 2) {
    std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
    if (mpiCommunicator_.rank() == mpiRankOwner_) {
      std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                            fileData.getData().getContainers();
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
                                           const FileData& fileData,
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm) {
  oops::Log::trace() << "Reader::getFieldMetadata()" << std::endl;
  // No MPI rank check - used to call private functions that broadcast data to all PEs
  std::vector<constants::FieldMetadata> fieldMetadataVec;
  for (auto lfricIt = lfricFieldNames.begin(), atlasIt = atlasFieldNames.begin();
                            lfricIt != lfricFieldNames.end(); ++lfricIt , ++atlasIt) {
    struct constants::FieldMetadata fieldMetadata;
    fieldMetadata.lfricName = *lfricIt;
    fieldMetadata.atlasName = *atlasIt;
    fieldMetadata.numLevels = getVarNumLevels(fileData, fieldMetadata.lfricName, levelsSearchTerm);
    fieldMetadata.dataType = getVarDataType(fileData, fieldMetadata.lfricName);
    fieldMetadata.fieldSize = getSizeOwned(fileData, fieldMetadata.lfricName);
    fieldMetadataVec.push_back(fieldMetadata);
  }
  return fieldMetadataVec;
}

size_t monio::Reader::getSizeOwned(const FileData& fileData, const std::string& varName) {
  oops::Log::debug() << "Reader::getSizeOwned()" << std::endl;
  size_t totalSize;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
    totalSize = variable->getTotalSize();
  }
  mpiCommunicator_.broadcast(totalSize, mpiRankOwner_);
  return totalSize;
}

size_t monio::Reader::getVarNumLevels(const FileData& fileData,
                                      const std::string& varName,
                                      const std::string& levelsSearchTerm) {
  oops::Log::debug() << "Reader::getVarNumLevels()" << std::endl;
  size_t numLevels;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
    numLevels = variable->findDimensionSize(levelsSearchTerm);
  }
  mpiCommunicator_.broadcast(numLevels, mpiRankOwner_);
  return numLevels;
}

int monio::Reader::getVarDataType(const FileData& fileData, const std::string& varName) {
  oops::Log::debug() << "Reader::getVarDataType()" << std::endl;
  int dataType;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
    dataType = variable->getType();
  }
  mpiCommunicator_.broadcast(dataType, mpiRankOwner_);
  return dataType;
}

size_t monio::Reader::findTimeStep(const FileData& fileData, const util::DateTime& dateTime) {
  oops::Log::debug() << "Reader::findTimeStep()" << std::endl;
  if (fileData.getDateTimes().size() == 0)
    throw std::runtime_error("Reader::findTimeStep()> Date times not initialised...");

  for (size_t timeStep = 0; timeStep < fileData.getDateTimes().size(); ++timeStep) {
    if (fileData.getDateTimes()[timeStep] == dateTime)
      return timeStep;
  }
  return -1;
}
