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
#include "oops/util/DateTime.h"

#include "Data.h"
#include "DataContainerBase.h"
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
  ~Reader();

  Reader()                         = delete;  //!< Deleted default constructor
  Reader(const Reader&)            = delete;  //!< Deleted copy constructor
  Reader(Reader&&)                 = delete;  //!< Deleted move constructor

  Reader& operator=(const Reader&) = delete;  //!< Deleted copy assignment
  Reader& operator=(Reader&&)      = delete;  //!< Deleted move assignment

  void readMetadata();
  void readVariablesData();
  void readVariable(const std::string varName);

  void createDateTimes(const std::string& timeVarName, const std::string& timeOriginName);

  void readFieldData(const std::vector<std::string>& variableNames,
                     const std::string& dateString,
                     const std::string& timeDimName);

  void readFieldData(const std::vector<std::string>& variableNames,
                     const util::DateTime dateToRead,
                     const std::string& timeDimName);

  std::vector<std::string> getVarStrAttrs(const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  std::map<std::string, std::shared_ptr<DataContainerBase>> getCoordMap(
                                           const std::vector<std::string>& coordNames);
  // The following function takes a levels 'search term' as some variables use full- or half-levels
  // This approach allows the correct number of levels for the variable to be determined
  std::map<std::string, std::tuple<std::string, int, size_t>> getFieldToMetadataMap(
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm);

  void deleteDimension(const std::string& dimName);
  void deleteVariable(const std::string& varName);

  Metadata& getMetadata();
  Data& getData();

 private:
  int getVarDataType(const std::string& varName);
  size_t getVarNumLevels(const std::string& varName, const std::string& levelsSearchTerm);

  void readField(const std::string varName,
                 const util::DateTime dateToRead,
                 const std::string timeDimName);
  size_t findTimeStep(const util::DateTime dateTime);

  std::shared_ptr<File> getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  std::shared_ptr<File> file_;
  Metadata metadata_;
  Data data_;

  std::vector<util::DateTime> dateTimes_;
};
}  // namespace monio
