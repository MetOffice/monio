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
                      const std::string& filePath):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner),
  data_(),
  metadata_() {
  oops::Log::debug() << "Reader::Reader()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    try {
      file_ = std::make_shared<File>(filePath, netCDF::NcFile::read);
    } catch (netCDF::exceptions::NcException& exception) {
      std::string message =
          "Reader::Reader()> An exception occurred while creating File...";
      message.append(exception.what());
      throw std::runtime_error(message);
    }
  }
}

void monio::Reader::readMetadata() {
  oops::Log::debug() << "Reader::readMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile()->readMetadata(metadata_);
  }
}

void monio::Reader::readAllData() {
  oops::Log::debug() << "Reader::readVariableData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<std::string> varNames = metadata_.getVariableNames();
    for (const auto& varName : varNames) {
      readSingleDatum(varName);
    }
  }
}

void monio::Reader::createDateTimes(const std::string& timeVarName,
                                    const std::string& timeOriginName) {
  oops::Log::debug() << "Reader::createDateTimes()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (dateTimes_.size() != 0)
      throw std::runtime_error("Reader::createDateTimes()> "
                               "Date times already initialised...");

    std::shared_ptr<Variable> timeVar = metadata_.getVariable(timeVarName);
    std::shared_ptr<DataContainerBase> timeDataBase = data_.getContainer(timeVarName);
    if (timeDataBase->getType() != constants::eDouble)
      throw std::runtime_error("Reader::createDateTimes()> "
                               "Time data not stored as double...");

    std::shared_ptr<DataContainerDouble> timeData =
                std::static_pointer_cast<DataContainerDouble>(timeDataBase);

    std::string timeOrigin = timeVar->getStrAttr(timeOriginName);
    std::string timeAtlasOrigin = convertToAtlasDateTimeStr(timeOrigin);

    util::DateTime originDateTime(timeAtlasOrigin);

    oops::Log::debug() << "timeVar->getSize()> " << timeVar->getTotalSize() << std::endl;
    dateTimes_.resize(timeVar->getTotalSize());
    for (std::size_t index = 0; index < timeVar->getTotalSize(); ++index) {
      util::Duration duration(static_cast<uint64_t>(std::round(timeData->getDatum(index))));
      util::DateTime dateTime = originDateTime + duration;
      dateTimes_[index] = dateTime;
      oops::Log::debug() << "index> " << index << ", data> " << timeData->getDatum(index) <<
                           ", dateTime> " << dateTime << std::endl;
    }
  }
}

void monio::Reader::readFieldData(const std::vector<std::string>& varNames,
                                  const std::string& dateString,
                                  const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readFieldData()" << std::endl;
  util::DateTime dateToRead(dateString);
  readFieldData(varNames, dateToRead, timeDimName);
}

void monio::Reader::readFieldData(const std::vector<std::string>& varNames,
                                  const util::DateTime& dateToRead,
                                  const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readFieldData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    size_t timeStep = findTimeStep(dateToRead);
    for (auto& varName : varNames) {
      readFieldDatum(varName, timeStep, timeDimName);
    }
  }
}

void monio::Reader::readFieldDatum(const std::string& varName,
                                   const size_t timeStep,
                                   const std::string& timeDimName) {
      std::shared_ptr<DataContainerBase> dataContainer = nullptr;
      std::shared_ptr<Variable> variable = metadata_.getVariable(varName);
      int dataType = variable->getType();
      size_t varSizeNoTime = 1;

      std::vector<size_t> startVec;
      std::vector<size_t> countVec;

      startVec.push_back(timeStep);
      countVec.push_back(1);

      std::vector<std::pair<std::string, size_t>> dimensions = variable->getDimensions();
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
        data_.addContainer(dataContainer);
      else
        throw std::runtime_error("Reader::readFieldData()> "
            "An exception occurred while creating data container...");
}

void monio::Reader::readSingleDatum(const std::string& varName) {
  oops::Log::debug() << "Reader::readVariable()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    std::shared_ptr<Variable> variable = metadata_.getVariable(varName);
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
      data_.addContainer(dataContainer);
    else
      throw std::runtime_error("Reader::readVariable()> "
          "An exception occurred while creating data container...");
  }
}

size_t monio::Reader::findTimeStep(const util::DateTime& dateTime) {
  oops::Log::debug() << "Reader::findTimeStep()" << std::endl;
  if (dateTimes_.size() == 0)
    throw std::runtime_error("Reader::findTimeStep()> Date times not initialised...");

  for (size_t timeStep = 0; timeStep < dateTimes_.size(); ++timeStep) {
    if (dateTimes_[timeStep] == dateTime)
      return timeStep;
  }
  return -1;
}

std::shared_ptr<monio::File> monio::Reader::getFile() {
  oops::Log::debug() << "Reader::getFile()" << std::endl;
  if (file_ == nullptr)
    throw std::runtime_error("Reader::getFile()> File has not been initialised...");

  return file_;
}

std::vector<std::string> monio::Reader::getVarStrAttrs(const std::vector<std::string>& varNames,
                                                       const std::string& attrName) {
  oops::Log::debug() << "Reader::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varStrAttrs;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    varStrAttrs = metadata_.getVarStrAttrs(varNames, attrName);
  }
  return varStrAttrs;
}

std::vector<std::shared_ptr<monio::DataContainerBase>> monio::Reader::getCoordData(
                                                   const std::vector<std::string>& coordNames) {
  oops::Log::debug() << "Reader::getCoordData()" << std::endl;
  if (coordNames.size() == 2) {
    std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
    if (mpiCommunicator_.rank() == mpiRankOwner_) {
      std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                            data_.getContainers();
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

std::map<std::string, std::tuple<std::string, int, size_t>> monio::Reader::getFieldToMetadataMap(
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm) {
  oops::Log::trace() << "Reader::getMapOfVarsToDataTypes()" << std::endl;
  // No MPI rank check - used to call private functions that broadcast data to all PEs
  std::map<std::string, std::tuple<std::string, int, size_t>> fieldToMetadataMap;

  for (auto lfricIt = lfricFieldNames.begin(), atlasIt = atlasFieldNames.begin();
                            lfricIt != lfricFieldNames.end(); ++lfricIt , ++atlasIt) {
    std::string lfricFieldName = *lfricIt;
    std::string atlasFieldName = *atlasIt;
    int dataType = getVarDataType(lfricFieldName);
    size_t numLevels = getVarNumLevels(lfricFieldName, levelsSearchTerm);
    fieldToMetadataMap.insert({lfricFieldName, {atlasFieldName, dataType, numLevels}});
  }
  return fieldToMetadataMap;
}

int monio::Reader::getVarDataType(const std::string& varName) {
  oops::Log::debug() << "Reader::getVarDataType()" << std::endl;
  int dataType;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = metadata_.getVariable(varName);
    dataType = variable->getType();
  }
  mpiCommunicator_.broadcast(dataType, mpiRankOwner_);
  return dataType;
}

size_t monio::Reader::getVarNumLevels(const std::string& varName,
                                      const std::string& levelsSearchTerm) {
  oops::Log::debug() << "Reader::getVarDataType()" << std::endl;
  size_t numLevels;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<Variable> variable = metadata_.getVariable(varName);
    numLevels = variable->findDimensionSize(levelsSearchTerm);
  }
  mpiCommunicator_.broadcast(numLevels, mpiRankOwner_);
  return numLevels;
}

void monio::Reader::deleteDimension(const std::string& dimName) {
  oops::Log::debug() << "Reader::deleteDimension()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    metadata_.deleteDimension(dimName);
  }
}

void monio::Reader::deleteVariable(const std::string& varName) {
  oops::Log::debug() << "Reader::deleteVariable()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    metadata_.deleteVariable(varName);
    data_.deleteContainer(varName);
  }
}

const monio::Metadata& monio::Reader::getMetadata() const {
  return metadata_;
}

const monio::Data& monio::Reader::getData() const {
  return data_;
}

monio::Metadata& monio::Reader::getMetadata() {
  return metadata_;
}

monio::Data& monio::Reader::getData() {
  return data_;
}
