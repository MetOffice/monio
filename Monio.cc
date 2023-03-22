/*
 * (C) Crown Copyright 2022-2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "lfriclitejedi/IO/Monio.h"

#include <memory>
#include <vector>

#include "atlas/parallel/mpi/mpi.h"
#include "lfriclitejedi/IO/Constants.h"
#include "oops/util/Logger.h"

std::unique_ptr<monio::Monio> monio::Monio::this_ = nullptr;

void monio::Monio::readFile(const std::string& gridName,
                            const std::string& filePath,
                            const util::DateTime& date) {
  oops::Log::trace() << "Monio::readFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = createFileData(gridName, filePath, date);
    // Storage of read data isn't hugely important at this stage. However, keying and storing
    // against the grid name allows for data of different resolutions to be read and available
    // at the point of writing.
    reader_.openFile(fileData);
    reader_.readMetadata(fileData);
    reader_.readSingleData(fileData, monio::constants::kLfricCoordVarNames);
    reader_.readSingleDatum(fileData, monio::constants::kTimeVarName);
    reader_.createDateTimes(fileData, monio::constants::kTimeVarName,
                                      monio::constants::kTimeOriginName);
  }
}

void monio::Monio::readVarAndPopulateField(const std::string& gridName,
                                           const std::string& varName,
                                           const util::DateTime& date,
                                           const atlas::idx_t& levels,
                                           atlas::Field& globalField) {
  oops::Log::trace() << "Monio::readVarAndPopulateField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = getFileData(gridName);
    createLfricAtlasMap(fileData, globalField);
    reader_.readFieldDatum(fileData, varName, date, monio::constants::kTimeDimName);
    globalField.set_levels(levels);
    atlasProcessor_.populateFieldWithDataContainer(globalField,
                                                   fileData.getData().getContainer(varName),
                                                   fileData.getLfricAtlasMap());
  }
}

void monio::Monio::writeIncrementsFile(const std::string& gridName,
                                       const atlas::FieldSet fieldset,
                                       const std::vector<std::string>& varNames,
                                       const std::string& filePath) {
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = getFileData(gridName);
    atlasProcessor_.writeIncrementsToFile(fieldset, varNames, fileData, filePath);
  }
}

monio::Monio& monio::Monio::get() {
  oops::Log::trace() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = std::unique_ptr<Monio>(new Monio(atlas::mpi::comm(), constants::kMPIRankOwner));
  }
  return *this_;
}

monio::Reader& monio::Monio::getReader() {
  oops::Log::trace() << "Monio::getReader()" << std::endl;
  return get().reader_;
}

monio::AtlasProcessor& monio::Monio::getAtlasProcessor() {
  oops::Log::trace() << "Monio::getAtlasProcessor()" << std::endl;
  return get().atlasProcessor_;
}

void monio::Monio::createLfricAtlasMap(FileData& fileData, atlas::Field& globalField) {
  oops::Log::trace() << "Monio::createLfricAtlasMap()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getLfricAtlasMap().size() == 0) {
      reader_.readSingleData(fileData, constants::kLfricCoordVarNames);
      std::vector<std::shared_ptr<monio::DataContainerBase>> coordData =
                                reader_.getCoordData(fileData, constants::kLfricCoordVarNames);
      std::vector<atlas::PointLonLat> lfricCoords = atlasProcessor_.getLfricCoords(coordData);
      std::vector<atlas::PointLonLat> atlasCoords = atlasProcessor_.getAtlasCoords(globalField);
      fileData.setLfricAtlasMap(atlasProcessor_.createLfricAtlasMap(atlasCoords, lfricCoords));
    }
  }
}

monio::FileData& monio::Monio::createFileData(const std::string& gridName,
                                              const std::string& filePath,
                                              const util::DateTime& date) {
  oops::Log::debug() << "Monio::createFileData()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    filesData_.erase(gridName);
  }
  // Overwrite existing data
  filesData_.insert({gridName, FileData(filePath, date)});
  return filesData_.at(gridName);
}

monio::FileData& monio::Monio::getFileData(const std::string& gridName) {
  oops::Log::debug() << "Monio::getFileData()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    return it->second;
  }
  throw std::runtime_error("Monio::getFileData()> FileData with grid name \"" +
                           gridName + "\" not found...");
}

monio::Monio::Monio(const eckit::mpi::Comm& mpiCommunicator,
                    const atlas::idx_t mpiRankOwner) :
      mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner),
      reader_(mpiCommunicator, mpiRankOwner_),
      atlasProcessor_(mpiCommunicator, mpiRankOwner_) {
  oops::Log::trace() << "Monio::Monio()" << std::endl;
}
