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

monio::Monio& monio::Monio::get() {
  oops::Log::trace() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio();
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
  if (fileData.getLfricAtlasMap().size() == 0) {
    reader_.readSingleData(fileData, constants::kLfricCoordVarNames);
    std::vector<std::shared_ptr<monio::DataContainerBase>> coordData =
                              reader_.getCoordData(fileData, constants::kLfricCoordVarNames);
    std::vector<atlas::PointLonLat> lfricCoords = atlasProcessor_.getLfricCoords(coordData);
    std::vector<atlas::PointLonLat> atlasCoords = atlasProcessor_.getAtlasCoords(globalField);
    fileData.setLfricAtlasMap(atlasProcessor_.createLfricAtlasMap(atlasCoords, lfricCoords));
  }
}

monio::Monio::Monio() :
  reader_(atlas::mpi::comm(), constants::kMPIRankOwner),
  atlasProcessor_(atlas::mpi::comm(), constants::kMPIRankOwner) {
  oops::Log::trace() << "Monio::Monio()" << std::endl;
}
