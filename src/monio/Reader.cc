/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
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
#include "Utils.h"
#include "Variable.h"

#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

monio::Reader::Reader(const eckit::mpi::Comm& mpiCommunicator,
                      const int mpiRankOwner,
                      const std::string& filePath):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Reader::Reader()" << std::endl;
  openFile(filePath);
}

monio::Reader::Reader(const eckit::mpi::Comm& mpiCommunicator,
                      const int mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "Reader::Reader()" << std::endl;
}

void monio::Reader::openFile(const std::string& filePath) {
  oops::Log::debug() << "Reader::openFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (filePath.size() != 0) {
      try {
        file_ = std::make_unique<File>(filePath, netCDF::NcFile::read);
      } catch (netCDF::exceptions::NcException& exception) {
        closeFile();
        utils::throwException("Reader::openFile()> An exception occurred while accessing File...");
      }
    }
  }
}

void monio::Reader::closeFile() {
  oops::Log::debug() << "Reader::closeFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (isOpen() == true) {
      getFile().close();
      file_.release();
    }
  }
}

bool monio::Reader::isOpen() {
  return file_ != nullptr;
}

void monio::Reader::readMetadata(FileData& fileData) {
  oops::Log::debug() << "Reader::readMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile().readMetadata(fileData.getMetadata());
  }
}

void monio::Reader::readDatumAtTime(FileData& fileData,
                                   const std::string& varName,
                                   const util::DateTime& dateToRead,
                                   const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readDatumAtTime()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    size_t timeStep = findTimeStep(fileData, dateToRead);
    readDatumAtTime(fileData, varName, timeStep, timeDimName);
  }
}

void monio::Reader::readDatumAtTime(FileData& fileData,
                                   const std::string& varName,
                                   const size_t timeStep,
                                   const std::string& timeDimName) {
  oops::Log::debug() << "Reader::readDatumAtTime()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getData().isPresent(varName) == false) {
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
      std::shared_ptr<DataContainerBase> dataContainer = nullptr;
      switch (dataType) {
        case consts::eDataTypes::eDouble: {
          std::shared_ptr<DataContainerDouble> dataContainerDouble =
                                      std::make_shared<DataContainerDouble>(varName);
          dataContainerDouble->setSize(varSizeNoTime);
          getFile().readFieldDatum(varName, startVec, countVec, dataContainerDouble->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
          break;
        }
        case consts::eDataTypes::eFloat: {
          std::shared_ptr<DataContainerFloat> dataContainerFloat =
                                      std::make_shared<DataContainerFloat>(varName);
          dataContainerFloat->setSize(varSizeNoTime);
          getFile().readFieldDatum(varName, startVec, countVec, dataContainerFloat->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
          break;
        }
        case consts::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
                                      std::make_shared<DataContainerInt>(varName);
          dataContainerInt->setSize(varSizeNoTime);
          getFile().readFieldDatum(varName, startVec, countVec, dataContainerInt->getData());
          dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
          break;
        }
        default: {
          closeFile();
          utils::throwException("Reader::readDatumAtTime()> Data type not coded for...");
        }
      }
      if (dataContainer != nullptr) {
        fileData.getData().addContainer(dataContainer);
      } else {
        closeFile();
        utils::throwException("Reader::readDatumAtTime()> "
           "An exception occurred while creating data container...");
      }
    } else {
      oops::Log::debug() << "Reader::readDatumAtTime()> DataContainer \""
        << varName << "\" aleady defined." << std::endl;
    }
  }
}

void monio::Reader::readAllData(FileData& fileData) {
  oops::Log::debug() << "Reader::readAllData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<std::string> varNames = fileData.getMetadata().getVariableNames();
    readFullData(fileData, varNames);
  }
}

void monio::Reader::readFullData(FileData& fileData,
                                 const std::vector<std::string>& varNames) {
  oops::Log::debug() << "Reader::readFullData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    for (const auto& varName : varNames) {
      readFullDatum(fileData, varName);
    }
  }
}

void monio::Reader::readFullDatum(FileData& fileData,
                                  const std::string& varName) {
  oops::Log::debug() << "Reader::readFullDatum()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    std::shared_ptr<Variable> variable = fileData.getMetadata().getVariable(varName);
    int dataType = variable->getType();
    switch (dataType) {
      case consts::eDataTypes::eDouble: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                            std::make_shared<DataContainerDouble>(varName);
        dataContainerDouble->setSize(variable->getTotalSize());
        getFile().readSingleDatum(varName, dataContainerDouble->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
        break;
      }
      case consts::eDataTypes::eFloat: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                            std::make_shared<DataContainerFloat>(varName);
        dataContainerFloat->setSize(variable->getTotalSize());
        getFile().readSingleDatum(varName, dataContainerFloat->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
        break;
      }
      case consts::eDataTypes::eInt: {
        std::shared_ptr<DataContainerInt> dataContainerInt =
                            std::make_shared<DataContainerInt>(varName);
        dataContainerInt->setSize(variable->getTotalSize());
        getFile().readSingleDatum(varName, dataContainerInt->getData());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
        break;
      }
      default: {
        closeFile();
        utils::throwException("Reader::readFullDatum()> Data type not coded for...");
      }
    }

    if (dataContainer != nullptr) {
      fileData.getData().addContainer(dataContainer);
    } else {
      closeFile();
      utils::throwException("Reader::readFullDatum()> "
          "An exception occurred while creating data container...");
    }
  }
}

monio::File& monio::Reader::getFile() {
  oops::Log::debug() << "Reader::getFile()" << std::endl;
  if (isOpen() == false) {
    utils::throwException("Reader::getFile()> File has not been initialised...");
  }
  return *file_;
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
        if (utils::findInVector(coordNames, dataPair.first) == true) {
          std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(dataPair.first);
          coordContainers.push_back(dataContainer);
        }
      }
    }
    return coordContainers;
  } else {
    closeFile();
    utils::throwException("Reader::getCoordData()> Incorrect number of coordinate axes...");
  }
}

size_t monio::Reader::findTimeStep(const FileData& fileData, const util::DateTime& dateTime) {
  oops::Log::debug() << "Reader::findTimeStep()" << std::endl;
  if (fileData.getDateTimes().size() == 0) {
    oops::Log::debug() << "Reader::findTimeStep()> Date times not initialised..." << std::endl;
    utils::throwException("Reader::findTimeStep()> Date times not initialised...");
  }

  for (size_t timeStep = 0; timeStep < fileData.getDateTimes().size(); ++timeStep) {
    if (fileData.getDateTimes()[timeStep] == dateTime) {
      return timeStep;
    }
  }
  closeFile();
  utils::throwException("Reader::findTimeStep()> DateTime specified not located in file...");
}
