/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#pragma once

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/grid/Iterator.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/meshgenerator/MeshGenerator.h"

#include "eckit/filesystem/LocalPathName.h"
#include "eckit/testing/Test.h"

#include "monio/Constants.h"
#include "monio/FileData.h"
#include "monio/Metadata.h"
#include "monio/Reader.h"
#include "monio/Writer.h"

#include "oops/../test/TestEnvironment.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"

namespace monio {
namespace test {
void testFunction() {
  oops::Log::info() << "basicWriteTest()" << std::endl;
  const eckit::LocalConfiguration inputConfig(::test::TestEnvironment::config(), "filePaths");
  eckit::LocalPathName inputFilePath  = inputConfig.getString("inputFilePath");
  eckit::LocalPathName outputFilePath = inputConfig.getString("outputFilePath");

  if (inputFilePath.exists() == false) {
    throw eckit::CantOpenFile(inputFilePath + " file not found...");
  } else {
    monio::FileData firstFileData(inputFilePath);

    monio::Reader reader(atlas::mpi::comm(),
                         monio::consts::kMPIRankOwner,
                         firstFileData);

    reader.readMetadata(firstFileData);
    reader.readAllData(firstFileData);

    monio::Writer writer(atlas::mpi::comm(),
                         monio::consts::kMPIRankOwner,
                         outputFilePath);
    writer.writeData(firstFileData.getMetadata(), firstFileData.getData());

    monio::FileData secondFileData(outputFilePath);
    reader.openFile(secondFileData);

    reader.readMetadata(secondFileData);
    if (firstFileData.getMetadata() == secondFileData.getMetadata()) {
      reader.readAllData(secondFileData);
      if (firstFileData.getData() == secondFileData.getData()) {
        oops::Log::info() << "basicWriteTest() passed." << std::endl;
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

class BackgroundBasic : public oops::Test{
 public:
  BackgroundBasic() {}
  virtual ~BackgroundBasic() {}
 private:
  std::string testid() const override {
    return "monio::test::BackgroundBasic";
  }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    std::function<void(std::string&, int&, int)> testFn =
        [](std::string &, int&, int) { testFunction(); };
    ts.push_back(eckit::testing::Test("monio/test_background_basic", testFn));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
