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
#include "monio/Monio.h"
#include "monio/Utils.h"

#include "oops/../test/TestEnvironment.h"
#include "oops/runs/Test.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace monio {
namespace test {

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
                               std::map<std::string, constants::VariableMetadata>& varMetadataMap) {
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

void compare() {
  oops::Log::info() << "monio::test::compare()" << std::endl;
}

/// Reads
void readOutput(const std::string& outputFilePath) {
  oops::Log::info() << "monio::test::readOutput()" << std::endl;
  oops::Log::info() << "outputFilePath> " << outputFilePath << std::endl;
}

/// Writes FieldSet to file.
void write(const std::string& outputFilePath) {
  oops::Log::info() << "monio::test::write()" << std::endl;
  oops::Log::info() << "outputFilePath> " << outputFilePath << std::endl;
}

/// Reads data from file and populates the FieldSet
void readInput(std::string& gridName,
               atlas::FieldSet& fieldSet,
               std::map<std::string, constants::VariableMetadata>& varMetadataMap,
               const util::DateTime& dateTime,
               const std::string& inputFilePath) {
  oops::Log::info() << "monio::test::readInput()" << std::endl;
  oops::Log::info() << "inputFilePath> " << inputFilePath << std::endl;
  oops::Log::info() << "dateTime> " << dateTime << std::endl;

  Monio::get().read(gridName, fieldSet, inputFilePath, dateTime);
}

/// Sets up the objects required to mimic an operational call to Monio::Read via readInput
void initParams(std::string& gridName,
                atlas::FieldSet& fieldSet,
                std::map<std::string, constants::VariableMetadata>& varMetadataMap,
                util::DateTime& dateTime,
                std::string& inputFilePath,
                std::string& outputFilePath) {
  oops::Log::info() << "monio::test::init()" << std::endl;
  // FieldSet
  const eckit::LocalConfiguration paramConfig(::test::TestEnvironment::config(), "parameters");
  gridName = paramConfig.getString("gridName");

  const std::string partitionerType(paramConfig.getString("partitionerType"));
  const std::string meshType(paramConfig.getString("meshType"));

  // Initialise Atlas objects to produce FieldSet
  atlas::CubedSphereGrid grid(gridName);
  atlas::Mesh mesh(createMesh(grid, partitionerType, meshType));
  atlas::functionspace::CubedSphereNodeColumns functionSpace(createFunctionSpace(mesh));

  fieldSet = createFieldSet(functionSpace, varMetadataMap);
  // VarMetadata
  const eckit::LocalConfiguration varMetadata = paramConfig.getSubConfiguration("varMetadata");
  for (const auto& key : varMetadata.keys()) {
    std::vector<std::string> stringVec = utils::strToWords(varMetadata.getString(key), ',');

    constants::VariableMetadata varMetadata;
    varMetadata.jediName = stringVec[constants::eJediName];
    varMetadata.lfricReadName = stringVec[constants::eLfricReadName];
    varMetadata.lfricWriteName = stringVec[constants::eLfricWriteName];
    varMetadata.units = stringVec[constants::eUnits];
    varMetadata.numberOfLevels =
                std::stoi(utils::strNoWhiteSpace(stringVec[constants::eNumberOfLevels]));
    varMetadata.copyFirstLevel = utils::strToBool(stringVec[constants::eCopyFirstLevel]);

    varMetadataMap.insert({key, varMetadata});
  }
  // Others
  dateTime = util::DateTime(paramConfig.getString("dateTime"));
  inputFilePath = paramConfig.getString("inputFilePath");
  outputFilePath = paramConfig.getString("outputFilePath");
}

void main() {
  std::string gridName;
  atlas::FieldSet fieldSet;
  std::map<std::string, constants::VariableMetadata> varMetadataMap;
  util::DateTime dateTime;
  std::string inputFilePath;
  std::string outputFilePath;
  initParams(gridName, fieldSet, varMetadataMap, dateTime, inputFilePath, outputFilePath);

  readInput(gridName, fieldSet, varMetadataMap, dateTime, inputFilePath);
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

    std::function<void(std::string&, int&, int)> mainFunction =
        [&](std::string&, int&, int) { main(); };
    ts.push_back(eckit::testing::Test("monio/test_read_write", mainFunction));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
