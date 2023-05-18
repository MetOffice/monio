/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/functionspace/PointCloud.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/grid/Iterator.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/meshgenerator/MeshGenerator.h"

#include "eckit/filesystem/LocalPathName.h"
#include "eckit/testing/Test.h"

#include "monio/AtlasData.h"
#include "monio/Constants.h"
#include "monio/Metadata.h"
#include "monio/Reader.h"
#include "monio/Writer.h"

#include "oops/../test/TestEnvironment.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"

namespace monio {
namespace test {
void incrementWriteTest() {
  oops::Log::info() << "incrementWriteTest()" << std::endl;
  const eckit::LocalConfiguration inputConfig(::test::TestEnvironment::config(), "filePaths");
  eckit::LocalPathName inputFilePath  = inputConfig.getString("inputFilePath");
  eckit::LocalPathName outputFilePath = inputConfig.getString("outputFilePath");
  std::string gridName = inputConfig.getString("gridName");
  std::string partitionerType = inputConfig.getString("partitionerType");
  std::string meshType = inputConfig.getString("meshType");
  std::vector<std::string> lfricCoordNames = inputConfig.getStringVector("lfricCoordNames");
  std::vector<std::string> lfricIncFieldNames = inputConfig.getStringVector("lfricIncFieldNames");
  std::vector<std::string> atlasIncFieldNames = inputConfig.getStringVector("atlasIncFieldNames");

  if (inputFilePath.exists() == false) {
    throw eckit::CantOpenFile(inputFilePath + " file not found...");
  } else {
    monio::FileData firstFileData(inputFilePath);

    monio::Reader reader(atlas::mpi::comm(),
                         monio::constants::kMPIRankOwner,
                         firstFileData);
    reader.readMetadata(firstFileData);
    reader.readAllData(firstFileData);

    monio::Writer writer(atlas::mpi::comm(),
                             monio::constants::kMPIRankOwner,
                             outputFilePath);

    monio::AtlasData atlasData(atlas::mpi::comm(),
                               monio::constants::kMPIRankOwner,
                               reader.getFieldMetadata(firstFileData,
                                                       lfricIncFieldNames,
                                                       atlasIncFieldNames,
                                                       monio::constants::kLevelsSearchTerm),
                               reader.getCoordData(firstFileData, lfricCoordNames),
                               gridName,
                               partitionerType,
                               meshType);

    atlasData.initialiseMemberFieldSet();

    atlasData.toFieldSet(firstFileData.getData());
    atlasData.fromFieldSet(firstFileData.getData());

    writer.writeData(firstFileData.getMetadata(), firstFileData.getData());

    monio::FileData secondFileData(outputFilePath);
    reader.readMetadata(secondFileData);

    if (firstFileData.getMetadata() == secondFileData.getMetadata()) {
      reader.readAllData(secondFileData);
      if (firstFileData.getData() == secondFileData.getData()) {
        oops::Log::info() << "incrementWriteTest() passed." << std::endl;
      } else {
        throw eckit::Stop("firstFileData.getData() == "
                          "secondFileData.getData() do not match");
      }
    } else {
      throw eckit::Stop("firstFileData.getMetadata() == "
                        "secondFileData.getMetadata() do not match");
    }
  }
}

class IncrementWrite : public oops::Test{
 public:
  IncrementWrite() {}
  virtual ~IncrementWrite() {}
 private:
  std::string testid() const override {
    return "monio::test::IncrementWrite";
  }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    std::function<void(std::string&, int&, int)> testFn =
        [](std::string &, int&, int) { incrementWriteTest(); };
    ts.push_back(eckit::testing::Test("monio/incrementWriteTest", testFn));
  }
  void clear() const override {}
};

}  // namespace test
}  // namespace monio
