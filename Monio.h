/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>
#include <utility>

#include "AtlasProcessor.h"
#include "FileData.h"
#include "Reader.h"

namespace monio {
class Monio {
 public:
  static Monio& get();
  static Reader& getReader();
  static AtlasProcessor& getAtlasProcessor();

  void readFile(const std::string& filePath, const util::DateTime& date);
  void readVarAndPopulateField(const std::string& filePath,
                               const std::string& varName,
                               const util::DateTime& date,
                               const atlas::idx_t& levels,
                               atlas::Field& globalField);

  void createLfricAtlasMap(FileData& fileData, atlas::Field& globalField);

 private:
  Monio(const eckit::mpi::Comm& mpiCommunicator,
        const atlas::idx_t mpiRankOwner);

  FileData& getFileData(const std::string& filePath, const util::DateTime& date);

  static Monio* this_;

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;

  monio::Reader reader_;
  monio::AtlasProcessor atlasProcessor_;

  std::map<std::pair<std::string, util::DateTime>, monio::FileData> filesData_;
};
}  // namespace monio
