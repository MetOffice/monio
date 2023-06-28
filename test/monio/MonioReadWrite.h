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
void init() {
  oops::Log::info() << "monio::test::init()" << std::endl;
}

void readInput() {
  oops::Log::info() << "monio::test::readInput()" << std::endl;
}

void write() {
  oops::Log::info() << "monio::test::write()" << std::endl;
}

void readOutput() {
  oops::Log::info() << "monio::test::readOutput()" << std::endl;
}

void compare() {
  oops::Log::info() << "monio::test::compare()" << std::endl;
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

    std::function<void(std::string&, int&, int)> initFunction =
        [](std::string &, int&, int) { init(); };
    ts.push_back(eckit::testing::Test("monio/init", initFunction));

    std::function<void(std::string&, int&, int)> readInputFunction =
        [](std::string &, int&, int) { readInput(); };
    ts.push_back(eckit::testing::Test("monio/readInput", readInputFunction));

    std::function<void(std::string&, int&, int)> writeFunction =
        [](std::string &, int&, int) { write(); };
    ts.push_back(eckit::testing::Test("monio/write", writeFunction));

    std::function<void(std::string&, int&, int)> readOutputFunction =
        [](std::string &, int&, int) { readOutput(); };
    ts.push_back(eckit::testing::Test("monio/readOutput", readOutputFunction));

    std::function<void(std::string&, int&, int)> compareFunction =
        [](std::string &, int&, int) { compare(); };
    ts.push_back(eckit::testing::Test("monio/compare", compareFunction));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
