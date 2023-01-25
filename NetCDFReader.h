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

#include "NetCDFData.h"
#include "NetCDFDataContainerBase.h"
#include "NetCDFFile.h"
#include "NetCDFMetadata.h"

namespace lfriclite {
/// \brief Top-level class uses NetCDFFile, NetCDFMetadata,
/// and NetCDFData to read from a NetCDF file
class NetCDFReader {
 public:
  NetCDFReader(const eckit::mpi::Comm& mpiCommunicator,
               const atlas::idx_t mpiRankOwner,
               const std::string& filePath);
  ~NetCDFReader();

  NetCDFReader()                    = delete;  //!< Deleted default constructor
  NetCDFReader(const NetCDFReader&) = delete;  //!< Deleted copy constructor
  NetCDFReader(NetCDFReader&&)      = delete;  //!< Deleted move constructor

  NetCDFReader& operator=(const NetCDFReader&) = delete;  //!< Deleted copy assign
  NetCDFReader& operator=(NetCDFReader&&) = delete;       //!< Deleted move assign

  void createDateTimes(const std::string& timeVarName, const std::string& timeOriginName);

  void readFieldData(const std::vector<std::string>& variableNames,
                     const std::string& dateString,
                     const std::string& timeDimName);

  std::vector<std::string> getVarStrAttrs(const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  std::map<std::string, NetCDFDataContainerBase*> getCoordMap(
                                           const std::vector<std::string>& coordNames);
  std::map<std::string, std::tuple<std::string, int, size_t>> getFieldToMetadataMap(
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm);

  void deleteDimension(const std::string& dimName);
  void deleteVariable(const std::string& varName);

  NetCDFMetadata* getMetadata();
  NetCDFData* getData();

 private:
  void readMetadata();
  void readVariablesData();
  int getVarDataType(const std::string& varName);
  size_t getVarNumLevels(const std::string& varName, const std::string& levelsSearchTerm);

  void readVariable(const std::string varName);
  void readField(const std::string varName,
                 const std::string dateString,
                 const std::string timeDimName);
  size_t findTimeStep(const util::DateTime dateTime);

  NetCDFFile* getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  std::unique_ptr<NetCDFFile> file_;
  NetCDFMetadata metadata_;
  NetCDFData data_;

  std::vector<util::DateTime> dateTimes_;
};
}  // namespace lfriclite
