/*
 * (C) Crown Copyright 2022-2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "lfriclitejedi/IO/Monio.h"

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Logger.h"

monio::Monio* monio::Monio::this_ = nullptr;

monio::Monio* monio::Monio::get() {
  oops::Log::trace() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio();
  }
  return this_;
}

monio::Monio::Monio() :
  reader_(atlas::mpi::comm(), constants::kMPIRankOwner),
  atlasProcessor_(atlas::mpi::comm(), constants::kMPIRankOwner) {
  oops::Log::trace() << "Monio::Monio()" << std::endl;
}

const monio::Reader& monio::Monio::getReader() const {
  oops::Log::trace() << "Monio::getReader()" << std::endl;
  return reader_;
}

monio::Reader& monio::Monio::getReader() {
  oops::Log::trace() << "Monio::getReader()" << std::endl;
  return reader_;
}

const monio::AtlasProcessor& monio::Monio::getAtlasProcessor() const {
  oops::Log::trace() << "Monio::getAtlasProcessor()" << std::endl;
  return atlasProcessor_;
}
