/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "Data.h"
#include "File.h"
#include "Metadata.h"

namespace monio {
/// \brief Top-level class uses File, Metadata, AtlasData,
/// and Data to write to a NetCDF file
class Writer {
 public:
  explicit Writer(const eckit::mpi::Comm& mpiCommunicator,
                  const atlas::idx_t& mpiRankOwner,
                  const std::string& filePath);

  explicit Writer(const eckit::mpi::Comm& mpiCommunicator,
                  const atlas::idx_t& mpiRankOwner);

  ~Writer();

  Writer()                         = delete;  //!< Deleted default constructor
  Writer(const Writer&)            = delete;  //!< Deleted copy constructor
  Writer(Writer&&)                 = delete;  //!< Deleted move constructor

  Writer& operator=(const Writer&) = delete;  //!< Deleted copy assign
  Writer& operator=(Writer&&)      = delete;  //!< Deleted move assign

  void initialiseAtlasObjects(
          const std::map<std::string, DataContainerBase*>& coordDataMap,
          const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
          const std::string& gridName,
          const std::string& partitionerType,
          const std::string& meshType);

  void writeMetadata(Metadata& metadata);
  void writeVariablesData(Metadata& metadata, Data& data);

 private:
  File* getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  std::unique_ptr<File> file_;
};
}  // namespace lfriclite
