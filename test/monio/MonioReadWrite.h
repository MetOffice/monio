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
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "eckit/testing/Test.h"

#include "monio/Constants.h"
#include "monio/Utils.h"

#include "oops/../test/TestEnvironment.h"
#include "oops/runs/Test.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace monio {
namespace test {

enum eVariableMetadata {
  eJediName,
  eCopyFirstLevel
};

struct VariableMetadata {
  std::string lfricName;
  std::string jediName;
  bool copyFirstLevel;
};

atlas::Mesh createMesh(const atlas::CubedSphereGrid& grid,
                       const std::string& partitionerType,
                       const std::string& meshType) {
  oops::Log::debug() << "monio::test::createMesh()" << std::endl;
  const auto meshConfig = atlas::util::Config("partitioner", partitionerType) |
                          atlas::util::Config("halo", 0);
  const auto meshGen = atlas::MeshGenerator(meshType, meshConfig);
  return meshGen.generate(grid);
}

atlas::functionspace::CubedSphereNodeColumns createFunctionSpace(const atlas::Mesh& csMesh) {
  oops::Log::debug() << "monio::test::createFunctionSpace()" << std::endl;
  const auto functionSpace = atlas::functionspace::CubedSphereNodeColumns(csMesh);
  return functionSpace;
}

atlas::FieldSet createFieldSet(const atlas::functionspace::CubedSphereNodeColumns& functionSpace,
                               const std::map<std::string, VariableMetadata>& varMetadataMap) {
  oops::Log::debug() << "monio::test::createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& varMetadata : varMetadataMap) {
    std::string jediName = varMetadata.second.jediName;
    size_t numLevels = constants::kVerticalFullSize;
    atlas::util::Config atlasOptions = atlas::option::name(jediName) |
                                       atlas::option::levels(numLevels);
    fieldSet.add(functionSpace.createField<double>(atlasOptions));
  }
  return fieldSet;
}

void readInput(atlas::FieldSet& fieldSetFromInput,
               std::map<std::string, VariableMetadata>& varMetadataMap,
               const util::DateTime& dateTime,
               const std::string& inputFilePath) {
  oops::Log::info() << "monio::test::readInput()" << std::endl;
  oops::Log::info() << "inputFilePath> " << inputFilePath << std::endl;
  oops::Log::info() << "dateTime> " << dateTime << std::endl;
}

void write(const std::string& outputFilePath) {
  oops::Log::info() << "monio::test::write()" << std::endl;
  oops::Log::info() << "outputFilePath> " << outputFilePath << std::endl;
}

void readOutput(const std::string& outputFilePath) {
  oops::Log::info() << "monio::test::readOutput()" << std::endl;
  oops::Log::info() << "outputFilePath> " << outputFilePath << std::endl;
}

void compare() {
  oops::Log::info() << "monio::test::compare()" << std::endl;
}

void init() {
  oops::Log::info() << "monio::test::init()" << std::endl;

  const eckit::LocalConfiguration paramConfig(::test::TestEnvironment::config(), "parameters");
  const eckit::LocalConfiguration varMetadata = paramConfig.getSubConfiguration("varMetadata");

  std::map<std::string, VariableMetadata> varMetadataMap;
  for (const auto& key : varMetadata.keys()) {
    std::vector<std::string> stringVec = utils::strToWords(varMetadata.getString(key), ',');

    VariableMetadata varMetadata;
    varMetadata.lfricName = key;
    varMetadata.jediName = stringVec[eJediName];
    varMetadata.copyFirstLevel = utils::strToBool(stringVec[eCopyFirstLevel]);

    varMetadataMap.insert({key, varMetadata});
  }

  const std::string gridName(paramConfig.getString("gridName"));
  const std::string partitionerType(paramConfig.getString("partitionerType"));
  const std::string meshType(paramConfig.getString("meshType"));

  const util::DateTime dateTime(paramConfig.getString("dateTime"));
  const std::string inputFilePath(paramConfig.getString("inputFilePath"));
  const std::string outputFilePath(paramConfig.getString("outputFilePath"));

  // Initialise Atlas objects to produce FieldSet
  atlas::CubedSphereGrid grid(gridName);
  atlas::Mesh mesh(createMesh(grid, partitionerType, meshType));
  atlas::functionspace::CubedSphereNodeColumns functionSpace(createFunctionSpace(mesh));

  atlas::FieldSet fieldSetFromInput = createFieldSet(functionSpace, varMetadataMap);

  readInput(fieldSetFromInput, varMetadataMap, dateTime, inputFilePath);
  write(outputFilePath);
  readOutput(outputFilePath);
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
        [&](std::string&, int&, int) { init(); };
    ts.push_back(eckit::testing::Test("monio/init", initFunction));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
