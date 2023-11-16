/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "Monio.h"

#include <memory>
#include <vector>

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "Constants.h"
#include "Utils.h"
#include "UtilsAtlas.h"
#include "Writer.h"

namespace  {
  std::string convertToAtlasDateTimeStr(std::string lfricDateTimeStr) {
    std::vector<std::string> dateTimeSplit = monio::utils::strToWords(lfricDateTimeStr, ' ');
    return dateTimeSplit[0] + "T" + dateTimeSplit[1] + "Z";
  }
}

monio::Monio& monio::Monio::get() {
  oops::Log::debug() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio(atlas::mpi::comm(), consts::kMPIRankOwner);
  }
  return *this_;
}

monio::Monio* monio::Monio::this_ = nullptr;

monio::Monio::~Monio() {
  std::cout << "Monio::~Monio()" << std::endl;  // Uses std::cout by design
  delete this_;
}

void monio::Monio::readState(atlas::FieldSet& localFieldSet,
                            const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                            const std::string& filePath,
                            const util::DateTime& dateTime) {
  oops::Log::debug() << "Monio::readState()" << std::endl;
  if (localFieldSet.size() == 0) {
    Monio::get().closeFiles();
    utils::throwException("Monio::readState()> localFieldSet has zero fields...");
  }
  if (filePath.length() != 0) {
    if (utils::fileExists(filePath)) {
      try {
        for (const auto& fieldMetadata : fieldMetadataVec) {
          auto& localField = localFieldSet[fieldMetadata.jediName];
          atlas::Field globalField = utilsatlas::getGlobalField(localField);
          if (mpiCommunicator_.rank() == mpiRankOwner_) {
            auto& functionSpace = globalField.functionspace();
            auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();

            // Initialise file
            int namingConvention = initialiseFile(grid.name(), filePath, true);
            // getFileData returns a copy of FileData (with required LFRic mesh data), so read data
            // is discarded when FileData goes out-of-scope for reading subsequent fields.
            FileData fileData = getFileData(grid.name());
            // Configure read name
            std::string readName = fieldMetadata.lfricReadName;
            if (namingConvention == consts::eJediNaming) {
              readName = fieldMetadata.jediName;
            }
            oops::Log::debug() << "Monio::readState() processing data for> \"" <<
                                  readName << "\"..." << std::endl;
            // Read fields into memory
            reader_.readDatumAtTime(fileData, readName, dateTime,
                                    std::string(consts::kTimeDimName));
            atlasReader_.populateFieldWithFileData(globalField, fileData, fieldMetadata, readName);
          }
          auto& functionSpace = globalField.functionspace();
          functionSpace.scatter(globalField, localField);
          localField.haloExchange();
        }
        reader_.closeFile();
      } catch (netCDF::exceptions::NcException& exception) {
        Monio::get().closeFiles();
        std::string exceptionMessage = exception.what();
        utils::throwException("Monio::readState()> An exception has occurred: " + exceptionMessage);
      }
    } else {
      Monio::get().closeFiles();
      utils::throwException("Monio::readState()> File \"" + filePath + "\" does not exist...");
    }
  } else {
    Monio::get().closeFiles();
    utils::throwException("Monio::readState()> No file path supplied...");
  }
}

void monio::Monio::readIncrements(atlas::FieldSet& localFieldSet,
                            const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                            const std::string& filePath) {
  oops::Log::debug() << "Monio::readIncrements()" << std::endl;
  if (localFieldSet.size() == 0) {
    Monio::get().closeFiles();
    utils::throwException("Monio::readIncrements()> localFieldSet has zero fields...");
  }
  if (filePath.length() != 0) {
    if (utils::fileExists(filePath)) {
      try {
        for (const auto& fieldMetadata : fieldMetadataVec) {
          auto& localField = localFieldSet[fieldMetadata.jediName];
          atlas::Field globalField = utilsatlas::getGlobalField(localField);
          if (mpiCommunicator_.rank() == mpiRankOwner_) {
            auto& functionSpace = globalField.functionspace();
            auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();

            // Initialise file
            int namingConvention = initialiseFile(grid.name(), filePath);
            // getFileData returns a copy of FileData (with required LFRic mesh data), so read data
            // is discarded when FileData goes out-of-scope for reading subsequent fields.
            FileData fileData = getFileData(grid.name());
            // Configure read name
            std::string readName = fieldMetadata.lfricReadName;
            if (namingConvention == consts::eJediNaming) {
              readName = fieldMetadata.jediName;
            }
            oops::Log::debug() << "Monio::readIncrements() processing data for> \"" <<
                                  readName << "\"..." << std::endl;
            // Read fields into memory
            reader_.readFullDatum(fileData, readName);
            atlasReader_.populateFieldWithFileData(globalField, fileData, fieldMetadata, readName);
          }
          auto& functionSpace = globalField.functionspace();
          functionSpace.scatter(globalField, localField);
          localField.haloExchange();
        }
        reader_.closeFile();
      } catch (netCDF::exceptions::NcException& exception) {
        Monio::get().closeFiles();
        std::string exceptionMessage = exception.what();
        utils::throwException("Monio::readIncrements()> An exception has occurred: " +
                              exceptionMessage);
      }
    } else {
      Monio::get().closeFiles();
      utils::throwException("Monio::readIncrements()> File \"" + filePath + "\" does not exist...");
    }
  } else {
    Monio::get().closeFiles();
    utils::throwException("Monio::readIncrements()> No file path supplied...");
  }
}

void monio::Monio::writeIncrements(const atlas::FieldSet& localFieldSet,
                                   const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                                   const std::string& filePath,
                                   const bool isLfricConvention) {
  oops::Log::debug() << "Monio::writeIncrements()" << std::endl;
  if (localFieldSet.size() == 0) {
    Monio::get().closeFiles();
    utils::throwException("Monio::writeIncrements()> localFieldSet has zero fields...");
  }
  if (filePath.length() != 0) {
    try {
      auto& functionSpace = localFieldSet[0].functionspace();
      auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();
      FileData fileData = getFileData(grid.name());
      cleanFileData(fileData);
      writer_.openFile(filePath);
      for (const auto& fieldMetadata : fieldMetadataVec) {
        auto& localField = localFieldSet[fieldMetadata.jediName];
        atlas::Field globalField = utilsatlas::getGlobalField(localField);
        if (mpiCommunicator_.rank() == mpiRankOwner_) {
          // Configure write name
          std::string writeName;
          if (isLfricConvention == true) {
            writeName = fieldMetadata.lfricWriteName;
          } else if (isLfricConvention == false && fieldMetadata.jediName == globalField.name()) {
            writeName = fieldMetadata.jediName;
          } else {
            Monio::get().closeFiles();
            utils::throwException("Monio::writeIncrements()> "
                                  "Field metadata configuration error...");
          }
          oops::Log::debug() << "Monio::writeIncrements() processing data for> \"" <<
                                writeName << "\"..." << std::endl;

          atlasWriter_.populateFileDataWithField(fileData,
                                                 globalField,
                                                 fieldMetadata,
                                                 writeName,
                                                 isLfricConvention);
          writer_.writeMetadata(fileData.getMetadata());
          writer_.writeData(fileData);
          fileData.clearData();  // Globalised field data no longer required
        }
      }
      writer_.closeFile();
    } catch (netCDF::exceptions::NcException& exception) {
      Monio::get().closeFiles();
      std::string exceptionMessage = exception.what();
      utils::throwException("Monio::writeIncrements()> An exception occurred: " + exceptionMessage);
    }
  } else {
      oops::Log::info() << "Monio::writeIncrements()> No file path supplied. "
                           "NetCDF writing will not take place..." << std::endl;
  }
}

void monio::Monio::writeState(const atlas::FieldSet& localFieldSet,
                              const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                              const std::string& filePath,
                              const bool isLfricConvention) {
  oops::Log::debug() << "Monio::writeState()" << std::endl;
  if (localFieldSet.size() == 0) {
    Monio::get().closeFiles();
    utils::throwException("Monio::writeState()> localFieldSet has zero fields...");
  }
  if (filePath.length() != 0) {
    try {
      auto& functionSpace = localFieldSet[0].functionspace();
      auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();
      FileData fileData = getFileData(grid.name());
      cleanFileData(fileData);
      writer_.openFile(filePath);
      for (const auto& fieldMetadata : fieldMetadataVec) {
        auto& localField = localFieldSet[fieldMetadata.jediName];
        atlas::Field globalField = utilsatlas::getGlobalField(localField);
        if (mpiCommunicator_.rank() == mpiRankOwner_) {
          // Configure write name
          std::string writeName;
          if (isLfricConvention == true) {
            writeName = fieldMetadata.lfricReadName;
          } else if (isLfricConvention == false && fieldMetadata.jediName == globalField.name()) {
            writeName = fieldMetadata.jediName;
          } else {
            Monio::get().closeFiles();
            utils::throwException("Monio::writeState()> Field metadata configuration error...");
          }
          oops::Log::debug() << "Monio::writeState() processing data for> \"" <<
                                  writeName << "\"..." << std::endl;

          atlasWriter_.populateFileDataWithField(fileData,
                                                 globalField,
                                                 fieldMetadata,
                                                 writeName,
                                                 isLfricConvention);
          writer_.writeMetadata(fileData.getMetadata());
          writer_.writeData(fileData);
          fileData.clearData();  // Globalised field data no longer required
        }
      }
      writer_.closeFile();
    } catch (netCDF::exceptions::NcException& exception) {
      Monio::get().closeFiles();
      std::string exceptionMessage = exception.what();
      utils::throwException("Monio::writeState()> An exception has occurred: " + exceptionMessage);
    }
  } else {
    oops::Log::info() << "Monio::writeState()> No file path supplied. "
                         "NetCDF writing will not take place..." << std::endl;
  }
}

void monio::Monio::writeFieldSet(const atlas::FieldSet& localFieldSet,
                                 const std::string& filePath) {
  oops::Log::debug() << "Monio::writeFieldSet()" << std::endl;
  if (localFieldSet.size() == 0) {
    Monio::get().closeFiles();
    utils::throwException("Monio::writeFieldSet()> localFieldSet has zero fields...");
  }
  if (filePath.length() != 0) {
    try {
      FileData fileData;  // Object needs to persist across fields for correct metadata creation
      writer_.openFile(filePath);
      for (const auto& localField : localFieldSet) {
        atlas::Field globalField = utilsatlas::getGlobalField(localField);
        if (mpiCommunicator_.rank() == mpiRankOwner_) {
          atlasWriter_.populateFileDataWithField(fileData, globalField, globalField.name());
          writer_.writeMetadata(fileData.getMetadata());
          writer_.writeData(fileData);
          fileData.clearData();  // Globalised field data no longer required
        }
      }
      writer_.closeFile();
    } catch (netCDF::exceptions::NcException& exception) {
      Monio::get().closeFiles();
      std::string exceptionMessage = exception.what();
      utils::throwException("Monio::writeFieldSet()> An exception occurred: " + exceptionMessage);
    }
  } else {
    oops::Log::info() << "Monio::writeFieldSet()> No file path supplied. "
                         "NetCDF writing will not take place..." << std::endl;
  }
}

void monio::Monio::closeFiles() {
  oops::Log::debug() << "Monio::closeFiles()" << std::endl;
  reader_.closeFile();
  writer_.closeFile();
}

int monio::Monio::initialiseFile(const atlas::Grid& grid,
                                 const std::string& filePath,
                                 bool doCreateDateTimes) {
  oops::Log::debug() << "Monio::initialiseFile()" << std::endl;
  int namingConvention = consts::eNotDefined;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = createFileData(grid.name(), filePath);
    reader_.openFile(filePath);
    reader_.readMetadata(fileData);
    // Read data
    std::vector<std::string> meshVars =
        fileData.getMetadata().findVariableNames(std::string(consts::kLfricMeshTerm));
    reader_.readFullData(fileData, meshVars);
    reader_.readFullDatum(fileData, std::string(consts::kVerticalFullName));
    reader_.readFullDatum(fileData, std::string(consts::kVerticalHalfName));
    // Process read data
    createLfricAtlasMap(fileData, grid);
    if (doCreateDateTimes == true) {
      reader_.readFullDatum(fileData, std::string(consts::kTimeVarName));
      createDateTimes(fileData,
                      std::string(consts::kTimeVarName),
                      std::string(consts::kTimeOriginName));
    }
    namingConvention = fileData.getMetadata().getNamingConvention();
  }
  return namingConvention;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

monio::Monio::Monio(const eckit::mpi::Comm& mpiCommunicator,
                    const int mpiRankOwner) :
      mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner),
      reader_(mpiCommunicator, mpiRankOwner_),
      writer_(mpiCommunicator, mpiRankOwner_),
      atlasReader_(mpiCommunicator, mpiRankOwner_),
      atlasWriter_(mpiCommunicator, mpiRankOwner_)   {
  oops::Log::debug() << "Monio::Monio()" << std::endl;
}

monio::FileData& monio::Monio::createFileData(const std::string& gridName,
                                              const std::string& filePath) {
  oops::Log::debug() << "Monio::createFileData()" << std::endl;
  auto it = filesData_.find(gridName);

  if (it != filesData_.end()) {
    filesData_.erase(gridName);
  }
  // Overwrite existing data
  filesData_.insert({gridName, FileData()});
  return filesData_.at(gridName);
}

void monio::Monio::createLfricAtlasMap(FileData& fileData, const atlas::CubedSphereGrid& grid) {
  oops::Log::debug() << "Monio::createLfricAtlasMap()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getLfricAtlasMap().size() == 0) {
      reader_.readFullData(fileData, consts::kLfricCoordVarNames);
      std::vector<std::shared_ptr<monio::DataContainerBase>> coordData =
                                reader_.getCoordData(fileData, consts::kLfricCoordVarNames);
      std::vector<atlas::PointLonLat> lfricCoords = utilsatlas::getLfricCoords(coordData);
      std::vector<atlas::PointLonLat> atlasCoords = utilsatlas::getAtlasCoords(grid);
      fileData.setLfricAtlasMap(utilsatlas::createLfricAtlasMap(atlasCoords, lfricCoords));
    }
  }
}

void monio::Monio::createDateTimes(FileData& fileData,
                             const std::string& timeVarName,
                             const std::string& timeOriginName) {
  oops::Log::debug() << "Monio::createDateTimes()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getDateTimes().size() == 0) {
      std::shared_ptr<Variable> timeVar = fileData.getMetadata().getVariable(timeVarName);
      std::shared_ptr<DataContainerBase> timeDataBase =
                                             fileData.getData().getContainer(timeVarName);
      if (timeDataBase->getType() != consts::eDouble) {
        Monio::get().closeFiles();
        utils::throwException("Monio::createDateTimes()> Time data not stored as double...");
      }
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
}

monio::FileData monio::Monio::getFileData(const std::string& gridName) {
  oops::Log::debug() << "Monio::getFileData()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    return FileData(it->second);
  }
  return FileData();  // This function is called by all PEs. A return is essential.
}

void monio::Monio::cleanFileData(FileData& fileData) {
  oops::Log::debug() << "Monio::cleanFileData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    fileData.getMetadata().clearGlobalAttributes();
    fileData.getMetadata().deleteDimension(std::string(consts::kTimeDimName));
    fileData.getMetadata().deleteDimension(std::string(consts::kTileDimName));
    fileData.getData().deleteContainer(std::string(consts::kTimeVarName));
    fileData.getData().deleteContainer(std::string(consts::kTileVarName));
    // Reconcile Metadata with Data
    std::vector<std::string> metadataVarNames = fileData.getMetadata().getVariableNames();
    std::vector<std::string> dataContainerNames = fileData.getData().getDataContainerNames();

    for (const auto& metadataVarName : metadataVarNames) {
      auto it = std::find(begin(dataContainerNames),
                          end(dataContainerNames), metadataVarName);
      if (it == std::end(dataContainerNames)) {
        fileData.getMetadata().deleteVariable(metadataVarName);
      }
    }
  }
}
