/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "Writer.h"

#include <netcdf>
#include <map>
#include <stdexcept>

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"

#include "oops/util/Logger.h"

monio::Writer::Writer(const eckit::mpi::Comm& mpiCommunicator,
                      const atlas::idx_t mpiRankOwner,
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

void monio::Writer::writeData(const Metadata& metadata, const Data& data) {
  writeMetadata(metadata);
  writeVariablesData(metadata, data);
}

void monio::Writer::writeMetadata(const Metadata& metadata) {
  oops::Log::debug() << "Writer::writeMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile().writeMetadata(metadata);
  }
}

void monio::Writer::writeVariablesData(const Metadata& metadata, const Data& data) {
  oops::Log::debug() << "Writer::writeVariablesData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    const std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainerMap =
                                                                     data.getContainers();
    for (const auto& dataContainerPair : dataContainerMap) {
      std::string varName = dataContainerPair.first;
      // Checks variable exists in metadata
      metadata.getVariable(varName);
      std::shared_ptr<DataContainerBase> dataContainer = dataContainerPair.second;
      int dataType = dataContainerPair.second->getType();
      switch (dataType) {
      case constants::eDataTypes::eDouble: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
            std::static_pointer_cast<DataContainerDouble>(dataContainer);
        getFile().writeSingleDatum(varName, dataContainerDouble->getData());
        break;
      }
      case constants::eDataTypes::eFloat: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
            std::static_pointer_cast<DataContainerFloat>(dataContainer);
        getFile().writeSingleDatum(varName, dataContainerFloat->getData());
        break;
      }
      case constants::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
            std::static_pointer_cast<DataContainerInt>(dataContainer);
        getFile().writeSingleDatum(varName, dataContainerInt->getData());
        break;
      }
      default:
        throw std::runtime_error("Writer::writeVariablesData()> Data type not coded for...");
      }
    }
  }
}

monio::File& monio::Writer::getFile() {
  oops::Log::debug() << "Writer::getFile()" << std::endl;
  if (file_ == nullptr)
    throw std::runtime_error("Writer::getFile()> File has not been initialised...");

  return *file_;
}
