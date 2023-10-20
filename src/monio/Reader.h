/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/DateTime.h"

#include "DataContainerBase.h"
#include "File.h"
#include "FileData.h"

namespace monio {
/// \brief Top-level class reads from a NetCDF file and populates instances of FileData.
class Reader {
 public:
  explicit Reader(const eckit::mpi::Comm& mpiCommunicator,
                  const int mpiRankOwner,
                  const std::string& filePath);

  explicit Reader(const eckit::mpi::Comm& mpiCommunicator,
                  const int mpiRankOwner);

  Reader()                         = delete;  //!< Deleted default constructor
  Reader(Reader&&)                 = delete;  //!< Deleted move constructor
  Reader(const Reader&)            = delete;  //!< Deleted copy constructor
  Reader& operator=(Reader&&)      = delete;  //!< Deleted move assignment
  Reader& operator=(const Reader&) = delete;  //!< Deleted copy assignment

  void openFile(const std::string& filePath);
  void closeFile();
  bool isOpen();

  void readMetadata(FileData& fileData);
  /// \brief Reads complete data for a set of variables defined in metadata.
  void readAllData(FileData& fileData);
  /// \brief Reads complete data for a set of variables.
  void readFullData(FileData& fileData,
                    const std::vector<std::string>& varNames);
  /// \brief Reads a complete data for a single variable.
  void readFullDatum(FileData& fileData, const std::string& varName);

  /// \brief Reads data for a single variable on a specific date. Makes call to derive time step.
  void readDatumAtTime(FileData& fileData,
                      const std::string& variableName,
                      const util::DateTime& dateToRead,
                      const std::string& timeDimName);

  /// \brief Reads data for a single variable at a particular time step.
  void readDatumAtTime(FileData& fileData,
                      const std::string& variableName,
                      const size_t timeStep,
                      const std::string& timeDimName);

  /// \brief Copies of coordinate data from the set of populated data containers.
  std::vector<std::shared_ptr<DataContainerBase>> getCoordData(FileData& fileData,
                                                  const std::vector<std::string>& coordNames);

 private:
  /// \brief Converts a date-time into a time step.
  size_t findTimeStep(const FileData& fileData, const util::DateTime& dateTime);

  File& getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  std::unique_ptr<File> file_;
};
}  // namespace monio
