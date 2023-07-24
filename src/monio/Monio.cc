/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "Monio.h"

#include <memory>
#include <vector>

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "Constants.h"
#include "Utils.h"

namespace  {
  std::string convertToAtlasDateTimeStr(std::string lfricDateTimeStr) {
    std::vector<std::string> dateTimeSplit = monio::utils::strToWords(lfricDateTimeStr, ' ');
    return dateTimeSplit[0] + "T" + dateTimeSplit[1] + "Z";
  }
}

monio::Monio* monio::Monio::this_ = nullptr;

monio::Monio::~Monio() {
  std::cout << "Monio::~Monio()" << std::endl;
  delete this_;
}

void monio::Monio::readBackground(atlas::FieldSet& localFieldSet,
                            const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                            const std::string& filePath,
                            const util::DateTime& dateTime) {
  oops::Log::debug() << "Monio::readBackground()" << std::endl;
  if (localFieldSet.size() == 0) {
    throw std::runtime_error("Monio::readBackground()> localFieldSet has zero fields...");
  }
  for (const auto& fieldMetadata : fieldMetadataVec) {
    auto& localField = localFieldSet[fieldMetadata.jediName];
    atlas::Field globalField = atlasProcessor_.getGlobalField(localField);
    if (mpiCommunicator_.rank() == mpiRankOwner_) {
      oops::Log::debug() << "Monio::readBackground() processing data for> \"" <<
                            fieldMetadata.jediName << "\"..." << std::endl;
      auto& functionSpace = globalField.functionspace();
      auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();

      // Initialise background file
      if (fileDataExists(grid.name()) == false) {
        FileData& fileData = createFileData(grid.name(), filePath, dateTime);
        reader_.openFile(fileData);
        reader_.readMetadata(fileData);
        std::vector<std::string> meshVars =
            fileData.getMetadata().findVariableNames(std::string(monio::consts::kLfricMeshTerm));
        reader_.readFullData(fileData, meshVars);
        createLfricAtlasMap(fileData, grid);

        reader_.readFullDatum(fileData, std::string(monio::consts::kTimeVarName));
        createDateTimes(fileData,
                        std::string(monio::consts::kTimeVarName),
                        std::string(monio::consts::kTimeOriginName));
      }
      FileData fileData = getFileData(grid.name());
      // Read fields into memory
      reader_.readDatumAtTime(fileData,
                              fieldMetadata.lfricReadName,
                              dateTime,
                              std::string(monio::consts::kTimeDimName));
      atlasReader_.populateFieldWithDataContainer(
                                       globalField,
                                       fileData.getData().getContainer(fieldMetadata.lfricReadName),
                                       fileData.getLfricAtlasMap(),
                                       fieldMetadata.copyFirstLevel);
    }
    auto& functionSpace = globalField.functionspace();
    functionSpace.scatter(globalField, localField);
    localField.haloExchange();
  }
}

void monio::Monio::readIncrements(atlas::FieldSet& localFieldSet,
                            const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                            const std::string& filePath) {
  oops::Log::debug() << "Monio::readIncrements()" << std::endl;
  if (localFieldSet.size() == 0) {
    throw std::runtime_error("Monio::readIncrements()> localFieldSet has zero fields...");
  }
  for (const auto& fieldMetadata : fieldMetadataVec) {
    auto& localField = localFieldSet[fieldMetadata.jediName];
    atlas::Field globalField = atlasProcessor_.getGlobalField(localField);
    if (mpiCommunicator_.rank() == mpiRankOwner_) {
      oops::Log::debug() << "Monio::readIncrements() processing data for> \"" <<
                            fieldMetadata.jediName << "\"..." << std::endl;
      auto& functionSpace = globalField.functionspace();
      auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();

      // Initialise background file
      if (fileDataExists(grid.name()) == false) {
        FileData& fileData = createFileData(grid.name(), filePath);
        reader_.openFile(fileData);
        reader_.readMetadata(fileData);
        std::vector<std::string> meshVars =
            fileData.getMetadata().findVariableNames(std::string(monio::consts::kLfricMeshTerm));
        reader_.readFullData(fileData, meshVars);
        createLfricAtlasMap(fileData, grid);
      }
      FileData fileData = getFileData(grid.name());
      // Read fields into memory
      reader_.readFullDatum(fileData, fieldMetadata.lfricReadName);
      atlasReader_.populateFieldWithDataContainer(
                                       globalField,
                                       fileData.getData().getContainer(fieldMetadata.lfricReadName),
                                       fileData.getLfricAtlasMap(),
                                       fieldMetadata.copyFirstLevel);
    }
    auto& functionSpace = globalField.functionspace();
    functionSpace.scatter(globalField, localField);
    localField.haloExchange();
  }
}

void monio::Monio::writeIncrements(const atlas::FieldSet& localFieldSet,
                                   const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                                   const std::string& filePath,
                                   const bool isLfricFormat) {
  oops::Log::debug() << "Monio::writeIncrements()" << std::endl;
  if (localFieldSet.size() == 0) {
    throw std::runtime_error("Monio::writeIncrements()> localFieldSet has zero fields...");
  }
  atlas::FieldSet globalFieldSet = atlasProcessor_.getGlobalFieldSet(localFieldSet);
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    auto& functionSpace = globalFieldSet[0].functionspace();
    auto& grid = atlas::functionspace::NodeColumns(functionSpace).mesh().grid();
    FileData fileData = getFileData(grid.name());
    atlasWriter_.writeIncrementsToFile(globalFieldSet, fieldMetadataVec,
                                       fileData, filePath, isLfricFormat);
  }
}

void monio::Monio::writeFieldSet(const atlas::FieldSet& localFieldSet,
                                 const std::string outputFilePath) {
  oops::Log::debug() << "Monio::writeFieldSet()" << std::endl;
  atlas::FieldSet globalFieldSet = atlasProcessor_.getGlobalFieldSet(localFieldSet);
  atlasWriter_.writeFieldSetToFile(globalFieldSet, outputFilePath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

monio::Monio& monio::Monio::get() {
  oops::Log::debug() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio(atlas::mpi::comm(), consts::kMPIRankOwner);
  }
  return *this_;
}

void monio::Monio::createLfricAtlasMap(FileData& fileData, const atlas::CubedSphereGrid& grid) {
  oops::Log::debug() << "Monio::createLfricAtlasMap()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getLfricAtlasMap().size() == 0) {
      reader_.readFullData(fileData, consts::kLfricCoordVarNames);
      std::vector<std::shared_ptr<monio::DataContainerBase>> coordData =
                                reader_.getCoordData(fileData, consts::kLfricCoordVarNames);
      std::vector<atlas::PointLonLat> lfricCoords = atlasProcessor_.getLfricCoords(coordData);
      std::vector<atlas::PointLonLat> atlasCoords = atlasProcessor_.getAtlasCoords(grid);
      fileData.setLfricAtlasMap(atlasProcessor_.createLfricAtlasMap(atlasCoords, lfricCoords));
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
      if (timeDataBase->getType() != consts::eDouble)
        throw std::runtime_error("Monio::createDateTimes()> "
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
}

monio::FileData& monio::Monio::createFileData(const std::string& gridName,
                                              const std::string& filePath,
                                              const util::DateTime& dateTime) {
  oops::Log::debug() << "Monio::createFileData()" << std::endl;
  auto it = filesData_.find(gridName);

  if (it != filesData_.end()) {
    filesData_.erase(gridName);
  }
  // Overwrite existing data
  filesData_.insert({gridName, FileData(filePath, dateTime)});
  return filesData_.at(gridName);
}

monio::FileData& monio::Monio::createFileData(const std::string& gridName,
                                              const std::string& filePath) {
  oops::Log::debug() << "Monio::createFileData()" << std::endl;
  auto it = filesData_.find(gridName);

  if (it != filesData_.end()) {
    filesData_.erase(gridName);
  }
  // Overwrite existing data
  filesData_.insert({gridName, FileData(filePath)});
  return filesData_.at(gridName);
}

monio::FileData monio::Monio::getFileData(const std::string& gridName) {
  oops::Log::debug() << "Monio::getFileData()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    return FileData(it->second);
  }
  throw std::runtime_error("Monio::getFileData()> FileData with grid name \"" +
                           gridName + "\" not found...");
}

bool monio::Monio::fileDataExists(const std::string& gridName) const {
  oops::Log::debug() << "Monio::fileDataExists()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    return true;
  }
  return false;
}

monio::Monio::Monio(const eckit::mpi::Comm& mpiCommunicator,
                    const atlas::idx_t mpiRankOwner) :
      mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner),
      reader_(mpiCommunicator, mpiRankOwner_),
      atlasProcessor_(mpiCommunicator, mpiRankOwner_),
      atlasReader_(mpiCommunicator, mpiRankOwner_),
      atlasWriter_(mpiCommunicator, mpiRankOwner_)   {
  oops::Log::debug() << "Monio::Monio()" << std::endl;
}
