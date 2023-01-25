/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFWriter.h"

#include <netcdf>
#include <map>
#include <stdexcept>

#include "NetCDFConstants.h"
#include "NetCDFDataContainerDouble.h"
#include "NetCDFDataContainerFloat.h"
#include "NetCDFDataContainerInt.h"

#include "oops/util/Logger.h"

lfriclite::NetCDFWriter::NetCDFWriter(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t& mpiRankOwner,
                                      const std::string& filePath)
    : mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "NetCDFWriter::NetCDFWriter()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    try {
      file_ = std::make_unique<NetCDFFile>(filePath, netCDF::NcFile::replace);
    } catch (netCDF::exceptions::NcException& exception) {
      std::string message =
          "NetCDFWriter::NetCDFWriter()> An exception occurred while creating NetCDFFile...";
      message.append(exception.what());
      throw std::runtime_error(message);
    }
  }
}

lfriclite::NetCDFWriter::NetCDFWriter(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t& mpiRankOwner)
    : mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "NetCDFWriter::NetCDFWriter()" << std::endl;
}

lfriclite::NetCDFWriter::~NetCDFWriter() {}

void lfriclite::NetCDFWriter::initialiseAtlasObjects(
                  const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap,
                  const std::map<std::string,
                        std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
                  const std::string& gridName,
                  const std::string& partitionerType,
                  const std::string& meshType) {
  oops::Log::debug() << "NetCDFWriter::initialiseAtlasObjects()" << std::endl;
  try {
    atlasData_ = std::make_unique<NetCDFAtlasData>(coordDataMap, fieldToMetadataMap, gridName,
                                                   partitionerType, meshType);
  } catch (netCDF::exceptions::NcException& exception) {
    std::string message = "NetCDFWriter::initialiseAtlasObjects()> "
                          "An exception occurred while creating NetCDFAtlasData...";
    message.append(exception.what());
    throw std::runtime_error(message);
  }
}

void lfriclite::NetCDFWriter::toAtlasFields(NetCDFData* data) {
  oops::Log::debug() << "NetCDFWriter::toAtlasFields()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getAtlasData()->toAtlasFields(data);
  }
  getAtlasData()->scatterAtlasFields();
}

void lfriclite::NetCDFWriter::fromAtlasFields(NetCDFData* data) {
  oops::Log::debug() << "NetCDFWriter::fromAtlasFields()" << std::endl;
  getAtlasData()->gatherAtlasFields();
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getAtlasData()->fromAtlasFields(data);
  }
}

void lfriclite::NetCDFWriter::writeMetadata(NetCDFMetadata* metadata) {
  oops::Log::debug() << "NetCDFWriter::writeMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile()->writeMetadata(*metadata);
  }
}

void lfriclite::NetCDFWriter::writeVariablesData(NetCDFMetadata* metadata, NetCDFData* data) {
  oops::Log::debug() << "NetCDFWriter::writeVariablesData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::map<std::string, NetCDFDataContainerBase*>& dataContainerMap = data->getContainers();
    for (const auto& dataContainerPair : dataContainerMap) {
      std::string varName = dataContainerPair.first;
      // Checks variable exists in metadata
      metadata->getVariable(varName);
      NetCDFDataContainerBase* dataContainer = dataContainerPair.second;
      int dataType = dataContainerPair.second->getType();
      switch (dataType) {
      case lfriclite::ncconsts::dataTypesEnum::eDouble: {
        NetCDFDataContainerDouble* dataContainerDouble =
            static_cast<NetCDFDataContainerDouble*>(dataContainer);
        getFile()->writeData(varName, dataContainerDouble->getData());
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eFloat: {
        NetCDFDataContainerFloat* dataContainerFloat =
            static_cast<NetCDFDataContainerFloat*>(dataContainer);
        getFile()->writeData(varName, dataContainerFloat->getData());
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eInt: {
        NetCDFDataContainerInt* dataContainerInt =
            static_cast<NetCDFDataContainerInt*>(dataContainer);
        getFile()->writeData(varName, dataContainerInt->getData());
        break;
      }
      default:
        throw std::runtime_error("NetCDFWriter::writeVariablesData()> Data type not coded for...");
      }
    }
  }
}

lfriclite::NetCDFFile* lfriclite::NetCDFWriter::getFile() {
  if (file_ == nullptr)
    throw std::runtime_error("NetCDFWriter::getFile()> File has not been initialised...");

  return file_.get();
}

lfriclite::NetCDFAtlasData* lfriclite::NetCDFWriter::getAtlasData() {
  if (atlasData_ == nullptr)
    throw std::runtime_error("NetCDFWriter::getAtlasData()> "
                             "Atlas data has not been initialised...");

  return atlasData_.get();
}
