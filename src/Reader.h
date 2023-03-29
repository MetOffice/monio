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

#include "DataContainerBase.h"
#include "Data.h"
#include "File.h"
#include "Metadata.h"

namespace monio {
/// \brief Top-level class uses File, Metadata,
/// and Data to read from a NetCDF file
class Reader {
 public:
  Reader(const eckit::mpi::Comm& mpiCommunicator,
         const atlas::idx_t mpiRankOwner,
         const std::string& filePath);

  Reader(const eckit::mpi::Comm& mpiCommunicator,
         const atlas::idx_t mpiRankOwner);

  Reader()                         = delete;  //!< Deleted default constructor
  Reader(Reader&&)                 = delete;  //!< Deleted move constructor
  Reader(const Reader&)            = delete;  //!< Deleted copy constructor
  Reader& operator=(Reader&&)      = delete;  //!< Deleted move assignment
  Reader& operator=(const Reader&) = delete;  //!< Deleted copy assignment

  void openFile(const std::string& filePath);

  void readMetadata(Metadata& metadata);
  void readAllData(Metadata& metadata, Data& data);

  void readSingleData(Metadata& metadata, Data& data, const std::vector<std::string>& varNames);
  void readSingleDatum(Metadata& metadata, Data& data, const std::string& varName);

  void readFieldDatum(Metadata& metadata,
                      Data& data,
                      const std::string& variableName,
                      const size_t timeStep,
                      const std::string& timeDimName);

  std::vector<std::string> getVarStrAttrs(const Metadata& metadata,
                                          const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  std::vector<std::shared_ptr<DataContainerBase>> getCoordData(Data& data,
                                                  const std::vector<std::string>& coordNames);
  // The following function takes a levels 'search term' as some variables use full- or half-levels
  // This approach allows the correct number of levels for the variable to be determined
  std::vector<monio::constants::FieldMetadata> getFieldMetadata(const Metadata& metadata,
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm);

 private:
  size_t getSizeOwned(const Metadata& metadata, const std::string& varName);
  size_t getVarNumLevels(const Metadata& metadata,
                         const std::string& varName,
                         const std::string& levelsSearchTerm);
  int getVarDataType(const Metadata& metadata, const std::string& varName);

  std::shared_ptr<File> getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  std::shared_ptr<File> file_;
};
}  // namespace monio
