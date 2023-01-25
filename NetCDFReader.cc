/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFReader.h"

#include <netcdf>
#include <map>
#include <sstream>
#include <stdexcept>

#include "NetCDFConstants.h"
#include "NetCDFDataContainerDouble.h"
#include "NetCDFDataContainerFloat.h"
#include "NetCDFDataContainerInt.h"
#include "NetCDFVariable.h"

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

lfriclite::NetCDFReader::NetCDFReader(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t mpiRankOwner,
                                      const std::string& filePath):
  mpiCommunicator_(mpiCommunicator),
  mpiRankOwner_(mpiRankOwner),
  data_(),
  metadata_() {
  oops::Log::debug() << "NetCDFReader::NetCDFReader()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    try {
      file_ = std::make_unique<NetCDFFile>(filePath, netCDF::NcFile::read);
      readMetadata();
      readVariablesData();
    } catch (netCDF::exceptions::NcException& exception) {
      std::string message =
          "NetCDFReader::NetCDFReader()> An exception occurred while creating NetCDFFile...";
      message.append(exception.what());
      throw std::runtime_error(message);
    }
  }
}

lfriclite::NetCDFReader::~NetCDFReader() {}

void lfriclite::NetCDFReader::readMetadata() {
  oops::Log::debug() << "NetCDFReader::readMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile()->readMetadata(metadata_);
  }
}

void lfriclite::NetCDFReader::readVariablesData() {
  oops::Log::debug() << "NetCDFReader::readVariableData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<std::string> varNames = metadata_.getVariableNames();
    for (const auto& varName : varNames) {
      readVariable(varName);
    }
  }
}

void lfriclite::NetCDFReader::createDateTimes(const std::string& timeVarName,
                                              const std::string& timeOriginName) {
  oops::Log::debug() << "NetCDFReader::createDateTimes()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (dateTimes_.size() != 0)
      throw std::runtime_error("NetCDFReader::createDateTimes()> "
                               "Date times already initialised...");

    NetCDFVariable* timeVar = metadata_.getVariable(timeVarName);
    NetCDFDataContainerBase* timeDataBase = data_.getContainer(timeVarName);
    if (timeDataBase->getType() != ncconsts::eDouble)
      throw std::runtime_error("NetCDFReader::createDateTimes()> "
                               "Time data not stored as double...");

    NetCDFDataContainerDouble* timeData = static_cast<NetCDFDataContainerDouble*>(timeDataBase);

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

void lfriclite::NetCDFReader::readFieldData(const std::vector<std::string>& fieldNames,
                                            const std::string& dateString,
                                            const std::string& timeDimName) {
  oops::Log::debug() << "NetCDFReader::readFieldData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    for (auto& fieldName : fieldNames) {
      readField(fieldName, dateString, timeDimName);
    }
  }
}

void lfriclite::NetCDFReader::readVariable(const std::string varName) {
  oops::Log::debug() << "NetCDFReader::readVariable()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    NetCDFDataContainerBase* dataContainer = nullptr;
    NetCDFVariable* variable = metadata_.getVariable(varName);
    int dataType = variable->getType();
    switch (dataType) {
      case lfriclite::ncconsts::dataTypesEnum::eDouble: {
        NetCDFDataContainerDouble* dataContainerDouble = new NetCDFDataContainerDouble(varName);
        getFile()->readData(varName,  variable->getTotalSize(), dataContainerDouble->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerDouble);
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eFloat: {
        NetCDFDataContainerFloat* dataContainerFloat = new NetCDFDataContainerFloat(varName);
        getFile()->readData(varName,  variable->getTotalSize(), dataContainerFloat->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerFloat);
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eInt: {
        NetCDFDataContainerInt* dataContainerInt = new NetCDFDataContainerInt(varName);
        getFile()->readData(varName,  variable->getTotalSize(), dataContainerInt->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerInt);
        break;
      }
      default:
        throw std::runtime_error("NetCDFReader::readVariable()> Data type not coded for...");
    }

    if (dataContainer != nullptr)
      data_.addContainer(dataContainer);
    else
      throw std::runtime_error("NetCDFReader::readVariable()> "
          "An exception occurred while creating data container...");
  }
}

void lfriclite::NetCDFReader::readField(const std::string varName,
                                        const std::string dateString,
                                        const std::string timeDimName) {
  oops::Log::debug() << "NetCDFReader::readField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    NetCDFDataContainerBase* dataContainer = nullptr;
    NetCDFVariable* variable = metadata_.getVariable(varName);
    int dataType = variable->getType();
    size_t varSizeNoTime = 1;

    size_t timeStep = findTimeStep(util::DateTime(dateString));

    std::vector<size_t> startVec;
    std::vector<size_t> countVec;

    startVec.push_back(timeStep);
    countVec.push_back(1);

    std::map<std::string, size_t> dimMap = variable->getDimensions();
    for (auto const& dimPair : dimMap) {
      if (dimPair.first != timeDimName) {
        varSizeNoTime *= dimPair.second;
        startVec.push_back(0);
        countVec.push_back(dimPair.second);
        oops::Log::debug() << "dimPair.first> " << dimPair.first <<
                            ", dimPair.second> " << dimPair.second << std::endl;
      }
    }
    switch (dataType) {
      case lfriclite::ncconsts::dataTypesEnum::eDouble: {
        NetCDFDataContainerDouble* dataContainerDouble = new NetCDFDataContainerDouble(varName);
        getFile()->readField(varName, varSizeNoTime,
                            startVec, countVec, dataContainerDouble->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerDouble);
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eFloat: {
        NetCDFDataContainerFloat* dataContainerFloat = new NetCDFDataContainerFloat(varName);
        getFile()->readField(varName, varSizeNoTime,
                             startVec, countVec, dataContainerFloat->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerFloat);
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eInt: {
        NetCDFDataContainerInt* dataContainerInt = new NetCDFDataContainerInt(varName);
        getFile()->readField(varName, varSizeNoTime,
                             startVec, countVec, dataContainerInt->getData());
        dataContainer = static_cast<NetCDFDataContainerBase*>(dataContainerInt);
        break;
      }
      default:
        throw std::runtime_error("NetCDFReader::readField()> Data type not coded for...");
    }
    if (dataContainer != nullptr)
      data_.addContainer(dataContainer);
    else
      throw std::runtime_error("NetCDFReader::readField()> "
          "An exception occurred while creating data container...");
  }
}

size_t lfriclite::NetCDFReader::findTimeStep(const util::DateTime dateTime) {
  if (dateTimes_.size() == 0)
    throw std::runtime_error("NetCDFReader::findTimeStep()> Date times not initialised...");

  for (size_t timeStep = 0; timeStep < dateTimes_.size(); ++timeStep) {
    if (dateTimes_[timeStep] == dateTime)
      return timeStep;
  }
  return -1;
}

lfriclite::NetCDFFile* lfriclite::NetCDFReader::getFile() {
    if (file_ == nullptr)
      throw std::runtime_error("NetCDFReader::getFile()> File has not been initialised...");

    return file_.get();
}

std::vector<std::string> lfriclite::NetCDFReader::getVarStrAttrs(
                                                  const std::vector<std::string>& varNames,
                                                  const std::string& attrName) {
  oops::Log::debug() << "NetCDFReader::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varStrAttrs;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    varStrAttrs = metadata_.getVarStrAttrs(varNames, attrName);
  }
  return varStrAttrs;
}

std::map<std::string, lfriclite::NetCDFDataContainerBase*>
                 lfriclite::NetCDFReader::getCoordMap(const std::vector<std::string>& coordNames) {
  oops::Log::debug() << "NetCDFReader::getCoordMap()" << std::endl;
  std::map<std::string, NetCDFDataContainerBase*> coordContainers;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::map<std::string, NetCDFDataContainerBase*>& dataContainers = data_.getContainers();

    for (auto& dataPair : dataContainers) {
      if (isStringInVector(dataPair.first, coordNames) == true) {
        NetCDFDataContainerBase* dataContainer = dataContainers[dataPair.first];
        coordContainers.insert({dataContainer->getName(), dataContainer});
      }
    }
  }
  return coordContainers;
}

std::map<std::string, std::tuple<std::string, int, size_t>>
                                      lfriclite::NetCDFReader::getFieldToMetadataMap(
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm) {
  oops::Log::debug() << "NetCDFReader::getMapOfVarsToDataTypes()" << std::endl;
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

int lfriclite::NetCDFReader::getVarDataType(const std::string& varName) {
  oops::Log::debug() << "NetCDFReader::getVarDataType()" << std::endl;
  int dataType;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    NetCDFVariable* variable = metadata_.getVariable(varName);
    dataType = variable->getType();
  }
  mpiCommunicator_.broadcast(dataType, mpiRankOwner_);
  return dataType;
}

size_t lfriclite::NetCDFReader::getVarNumLevels(const std::string& varName,
                                                const std::string& levelsSearchTerm) {
  oops::Log::debug() << "NetCDFReader::getVarDataType()" << std::endl;
  size_t numLevels;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    NetCDFVariable* variable = metadata_.getVariable(varName);
    numLevels = variable->findDimension(levelsSearchTerm);
  }
  mpiCommunicator_.broadcast(numLevels, mpiRankOwner_);
  return numLevels;
}

void lfriclite::NetCDFReader::deleteDimension(const std::string& dimName) {
  oops::Log::debug() << "NetCDFReader::deleteDimension()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    metadata_.deleteDimension(dimName);
  }
}

void lfriclite::NetCDFReader::deleteVariable(const std::string& varName) {
  oops::Log::debug() << "NetCDFReader::deleteVariable()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    metadata_.deleteVariable(varName);
    data_.deleteContainer(varName);
  }
}

lfriclite::NetCDFMetadata* lfriclite::NetCDFReader::getMetadata() {
  return &metadata_;
}

lfriclite::NetCDFData* lfriclite::NetCDFReader::getData() {
  return &data_;
}
