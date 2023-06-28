/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
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
                  const atlas::idx_t mpiRankOwner,
                  const std::string& filePath);

  Writer()                         = delete;  //!< Deleted default constructor
  Writer(Writer&&)                 = delete;  //!< Deleted move constructor
  Writer(const Writer&)            = delete;  //!< Deleted copy constructor
  Writer& operator=(Writer&&)      = delete;  //!< Deleted move assign
  Writer& operator=(const Writer&) = delete;  //!< Deleted copy assign

  void writeData(const Metadata& metadata, const Data& data);
  void writeMetadata(const Metadata& metadata);
  void writeVariablesData(const Metadata& metadata, const Data& data);

 private:
  File& getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  std::unique_ptr<File> file_;
};
}  // namespace monio