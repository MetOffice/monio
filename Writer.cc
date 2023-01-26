/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/Writer.h"

#include <netcdf>
#include <map>
#include <stdexcept>

#include "AtlasData.h"
#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"

#include "oops/util/Logger.h"

monio::Writer::Writer(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t& mpiRankOwner,
                      const std::string& filePath)
    : mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Writer::Writer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    try {
      file_ = std::make_unique<File>(filePath, netCDF::NcFile::replace);
    } catch (netCDF::exceptions::NcException& exception) {
      std::string message =
          "Writer::Writer()> An exception occurred while creating File...";
      message.append(exception.what());
      throw std::runtime_error(message);
    }
  }
}

monio::Writer::Writer(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t& mpiRankOwner)
    : mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Writer::Writer()" << std::endl;
}

monio::Writer::~Writer() {}

void monio::Writer::initialiseAtlasObjects(
                  const std::map<std::string, DataContainerBase*>& coordDataMap,
                  const std::map<std::string,
                        std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
                  const std::string& gridName,
                  const std::string& partitionerType,
                  const std::string& meshType) {
  oops::Log::debug() << "Writer::initialiseAtlasObjects()" << std::endl;
  try {
    atlasData_ = std::make_unique<AtlasData>(coordDataMap, fieldToMetadataMap, gridName,
                                                   partitionerType, meshType);
  } catch (netCDF::exceptions::NcException& exception) {
    std::string message = "Writer::initialiseAtlasObjects()> "
                          "An exception occurred while creating AtlasData...";
    message.append(exception.what());
    throw std::runtime_error(message);
  }
}

void monio::Writer::toAtlasFields(Data* data) {
  oops::Log::debug() << "Writer::toAtlasFields()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getAtlasData()->toAtlasFields(data);
  }
  getAtlasData()->scatterAtlasFields();
}

void monio::Writer::fromAtlasFields(Data* data) {
  oops::Log::debug() << "Writer::fromAtlasFields()" << std::endl;
  getAtlasData()->gatherAtlasFields();
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getAtlasData()->fromAtlasFields(data);
  }
}

void monio::Writer::writeMetadata(Metadata* metadata) {
  oops::Log::debug() << "Writer::writeMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile()->writeMetadata(*metadata);
  }
}

void monio::Writer::writeVariablesData(Metadata* metadata, Data* data) {
  oops::Log::debug() << "Writer::writeVariablesData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::map<std::string, DataContainerBase*>& dataContainerMap = data->getContainers();
    for (const auto& dataContainerPair : dataContainerMap) {
      std::string varName = dataContainerPair.first;
      // Checks variable exists in metadata
      metadata->getVariable(varName);
      DataContainerBase* dataContainer = dataContainerPair.second;
      int dataType = dataContainerPair.second->getType();
      switch (dataType) {
      case constants::dataTypesEnum::eDouble: {
        DataContainerDouble* dataContainerDouble =
            static_cast<DataContainerDouble*>(dataContainer);
        getFile()->writeData(varName, dataContainerDouble->getData());
        break;
      }
      case constants::dataTypesEnum::eFloat: {
        DataContainerFloat* dataContainerFloat =
            static_cast<DataContainerFloat*>(dataContainer);
        getFile()->writeData(varName, dataContainerFloat->getData());
        break;
      }
      case constants::dataTypesEnum::eInt: {
        DataContainerInt* dataContainerInt =
            static_cast<DataContainerInt*>(dataContainer);
        getFile()->writeData(varName, dataContainerInt->getData());
        break;
      }
      default:
        throw std::runtime_error("Writer::writeVariablesData()> Data type not coded for...");
      }
    }
  }
}

monio::File* monio::Writer::getFile() {
  if (file_ == nullptr)
    throw std::runtime_error("Writer::getFile()> File has not been initialised...");

  return file_.get();
}

monio::AtlasData* monio::Writer::getAtlasData() {
  if (atlasData_ == nullptr)
    throw std::runtime_error("Writer::getAtlasData()> "
                             "Atlas data has not been initialised...");

  return atlasData_.get();
}
