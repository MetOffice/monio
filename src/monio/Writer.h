/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "Data.h"
#include "File.h"
#include "FileData.h"
#include "Metadata.h"

namespace monio {
/// \brief Top-level class uses instances of FileData and their contents to write to a NetCDF file.
class Writer {
 public:
  explicit Writer(const eckit::mpi::Comm& mpiCommunicator,
                  const int mpiRankOwner,
                  const std::string& filePath);

  explicit Writer(const eckit::mpi::Comm& mpiCommunicator,
                  const int mpiRankOwner);

  Writer()                         = delete;  //!< Deleted default constructor
  Writer(Writer&&)                 = delete;  //!< Deleted move constructor
  Writer(const Writer&)            = delete;  //!< Deleted copy constructor
  Writer& operator=(Writer&&)      = delete;  //!< Deleted move assign
  Writer& operator=(const Writer&) = delete;  //!< Deleted copy assign

  void openFile(const std::string& filePath);
  void closeFile();
  bool isOpen();

  void writeMetadata(const Metadata& metadata);
  void writeData(const FileData& fileData);

 private:
  File& getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  std::unique_ptr<File> file_;
};
}  // namespace monio
