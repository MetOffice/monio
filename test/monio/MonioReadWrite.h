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
void read() {
  oops::Log::debug() << "readFile()" << std::endl;
}

void write() {
  oops::Log::debug() << "readFile()" << std::endl;
}

class MonioReadWrite : public oops::Test{
 public:
  MonioReadWrite() {}
  virtual ~MonioReadWrite() {}
 private:
  std::string testid() const override {
    return "monio::test::MonioReadWrite";
  }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    std::function<void(std::string&, int&, int)> readFunction =
        [](std::string &, int&, int) { read(); };
    ts.push_back(eckit::testing::Test("monio/read", readFunction));

    std::function<void(std::string&, int&, int)> writeFunction =
        [](std::string &, int&, int) { read(); };
    ts.push_back(eckit::testing::Test("monio/write", writeFunction));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
