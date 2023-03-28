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
#include <utility>
#include <vector>

#include "AtlasProcessor.h"
#include "FileData.h"
#include "Reader.h"

namespace monio {
class Monio {
 public:
  static Monio& get();
  static Reader& getReader();
  static AtlasProcessor& getAtlasProcessor();

  Monio()                        = delete;  //!< Deleted default constructor
  Monio(Monio&&)                 = delete;  //!< Deleted move constructor
  Monio(const Monio&)            = delete;  //!< Deleted copy constructor
  Monio& operator=(Monio&&)      = delete;  //!< Deleted move assignment
  Monio& operator=(const Monio&) = delete;  //!< Deleted copy assignment

  void readFile(const std::string& gridName,
                const std::string& filePath,
                const util::DateTime& date);

  void readVarAndPopulateField(const std::string& gridName,
                               const std::string& varName,
                               const util::DateTime& date,
                               const atlas::idx_t& levels,
                               atlas::Field& globalField);

  void writeIncrementsFile(const std::string& gridName,
                           const atlas::FieldSet fieldset,
                           const std::vector<std::string>& varNames,
                           const std::string& filePath);

  void createLfricAtlasMap(FileData& fileData, atlas::Field& globalField);

 private:
  Monio(const eckit::mpi::Comm& mpiCommunicator,
        const atlas::idx_t mpiRankOwner);

  FileData& createFileData(const std::string& gridName,
                           const std::string& filePath,
                           const util::DateTime& date);

  FileData& getFileData(const std::string& gridName);

  static std::unique_ptr<Monio> this_;

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  monio::Reader reader_;
  monio::AtlasProcessor atlasProcessor_;

  // Keyed by grid name for storage of data at different resolutions
  std::map<std::string, monio::FileData> filesData_;
};
}  // namespace monio
