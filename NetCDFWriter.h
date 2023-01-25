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

#include "NetCDFAtlasData.h"
#include "NetCDFData.h"
#include "NetCDFFile.h"
#include "NetCDFMetadata.h"

namespace lfriclite {
/// \brief Top-level class uses NetCDFFile, NetCDFMetadata, NetCDFAtlasData,
/// and NetCDFData to write to a NetCDF file
class NetCDFWriter {
 public:
  explicit NetCDFWriter(const eckit::mpi::Comm& mpiCommunicator,
                        const atlas::idx_t& mpiRankOwner,
                        const std::string& filePath);

  explicit NetCDFWriter(const eckit::mpi::Comm& mpiCommunicator,
                        const atlas::idx_t& mpiRankOwner);

  ~NetCDFWriter();

  NetCDFWriter() = delete;                     //!< Deleted default constructor
  NetCDFWriter(const NetCDFWriter&) = delete;  //!< Deleted copy constructor
  NetCDFWriter(NetCDFWriter&&) = delete;       //!< Deleted move constructor

  NetCDFWriter& operator=(const NetCDFWriter&) = delete;  //!< Deleted copy assign
  NetCDFWriter& operator=(NetCDFWriter&&) = delete;  //!< Deleted move assign

  void initialiseAtlasObjects(
          const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap,
          const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
          const std::string& gridName,
          const std::string& partitionerType,
          const std::string& meshType);
  void toAtlasFields(NetCDFData* data);
  void fromAtlasFields(NetCDFData* data);

  void writeMetadata(NetCDFMetadata* metadata);
  void writeVariablesData(NetCDFMetadata* metadata,
                          NetCDFData* data);

 private:
  NetCDFFile* getFile();
  NetCDFAtlasData* getAtlasData();

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  std::unique_ptr<NetCDFFile> file_;
  std::unique_ptr<NetCDFAtlasData> atlasData_;
};
}  // namespace lfriclite
