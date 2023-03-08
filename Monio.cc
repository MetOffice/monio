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

monio::Monio* monio::Monio::this_ = nullptr;

void monio::Monio::readFile(const std::string& filePath, const util::DateTime& date) {
  oops::Log::trace() << "Monio::readFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = getFileData(filePath, date);
    reader_.openFile(fileData);
    reader_.readMetadata(fileData);
    reader_.readSingleDatum(fileData, monio::constants::kTimeVarName);
    reader_.createDateTimes(fileData, monio::constants::kTimeVarName,
                                      monio::constants::kTimeOriginName);
  }
}

void monio::Monio::readVarAndPopulateField(const std::string& filePath,
                                           const std::string& varName,
                                           const util::DateTime& date,
                                           const atlas::idx_t& levels,
                                           atlas::Field& globalField) {
  oops::Log::trace() << "Monio::readVarAndPopulateField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData fileData = getFileData(filePath, date);
    createLfricAtlasMap(fileData, globalField);
    reader_.readFieldDatum(fileData, varName, date, monio::constants::kTimeDimName);
    globalField.set_levels(levels);
    atlasProcessor_.populateFieldWithDataContainer(globalField,
                                                   fileData.getData().getContainer(varName),
                                                   fileData.getLfricAtlasMap());
  }
}

monio::Monio& monio::Monio::get() {
  oops::Log::trace() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio(atlas::mpi::comm(), constants::kMPIRankOwner);
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

monio::FileData& monio::Monio::getFileData(const std::string& filePath,
                                           const util::DateTime& date) {
  oops::Log::debug() << "Monio::getFileData()" << std::endl;
  auto it = filesData_.find({filePath, date});
  if (it != filesData_.end()) {
    return it->second;
  } else {
    FileData fileData(filePath, date);
    auto retPair = filesData_.insert({{filePath, date}, fileData});
    return retPair.first->second;
  }
}

monio::Monio::Monio(const eckit::mpi::Comm& mpiCommunicator,
                    const atlas::idx_t mpiRankOwner) :
      mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner),
      reader_(mpiCommunicator, mpiRankOwner_),
      atlasProcessor_(mpiCommunicator, mpiRankOwner_) {
  oops::Log::trace() << "Monio::Monio()" << std::endl;
}
