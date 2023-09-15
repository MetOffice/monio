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
#include <utility>
#include <vector>

#include "AtlasReader.h"
#include "AtlasWriter.h"
#include "FileData.h"
#include "Reader.h"
#include "Writer.h"

namespace monio {
class Monio {
 public:
  static Monio& get();

  ~Monio();

  Monio()                        = delete;  //!< Deleted default constructor
  Monio(Monio&&)                 = delete;  //!< Deleted move constructor
  Monio(const Monio&)            = delete;  //!< Deleted copy constructor
  Monio& operator=(Monio&&)      = delete;  //!< Deleted move assignment
  Monio& operator=(const Monio&) = delete;  //!< Deleted copy assignment


  int initialiseFile(const atlas::Grid& grid,
                     const std::string& filePath,
                     const util::DateTime& dateTime);  // Public, whilst called from LFRic-Lite

  void readState(atlas::FieldSet& localFieldSet,
           const std::vector<consts::FieldMetadata>& fieldMetadataVec,
           const std::string& filePath,
           const util::DateTime& dateTime);

  void readIncrements(atlas::FieldSet& localFieldSet,
                const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                const std::string& filePath);

  void writeState(const atlas::FieldSet& localFieldSet,
                  const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                  const std::string& filePath);

  void writeIncrements(const atlas::FieldSet& localFieldSet,
                       const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                       const std::string& filePath,
                       const bool isLfricFormat = true);

  void writeFieldSet(const atlas::FieldSet& localFieldSet,
                     const std::string& filePath);

 private:
  Monio(const eckit::mpi::Comm& mpiCommunicator,
        const int mpiRankOwner);

  FileData& createFileData(const std::string& gridName,
                           const std::string& filePath,
                           const util::DateTime& dateTime);

  FileData& createFileData(const std::string& gridName,
                           const std::string& filePath);

  int initialiseFile(const atlas::Grid& grid,
                     const std::string& filePath);  // Public, whilst called from LFRic-Lite

  void createLfricAtlasMap(FileData& fileData, const atlas::CubedSphereGrid& grid);

  void createDateTimes(FileData& fileData,
                       const std::string& timeVarName,
                       const std::string& timeOriginName);

  FileData getFileData(const std::string& gridName);  // This function returns copies by design

  bool fileDataExists(const std::string& gridName) const;

  void cleanFileData(FileData& fileData);

  static Monio* this_;

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  Reader reader_;
  Writer writer_;

  AtlasReader atlasReader_;
  AtlasWriter atlasWriter_;

  // Keyed by grid name for storage of data at different resolutions
  std::map<std::string, monio::FileData> filesData_;
};
}  // namespace monio
