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
                               std::vector<consts::FieldMetadata>& varMetadataVec) {
  oops::Log::debug() << "monio::test::createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& varMetadata : varMetadataVec) {
    std::string jediName = varMetadata.jediName;
    size_t numLevels = consts::kVerticalFullSize;
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
void readInput(atlas::FieldSet& fieldSet,
               const std::vector<consts::FieldMetadata>& varMetadataVec,
               const util::DateTime& dateTime,
               const std::string& inputFilePath) {
  oops::Log::info() << "monio::test::readInput()" << std::endl;
  oops::Log::info() << "inputFilePath> " << inputFilePath << std::endl;
  oops::Log::info() << "dateTime> " << dateTime << std::endl;

  Monio::get().readBackground(fieldSet, varMetadataVec, inputFilePath, dateTime);
}

/// Sets up the objects required to mimic an operational call to Monio::Read via readInput
void initParams(atlas::FieldSet& fieldSet,
                std::vector<consts::FieldMetadata>& varMetadataVec,
                util::DateTime& dateTime,
                std::string& inputFilePath,
                std::string& outputFilePath) {
  oops::Log::info() << "monio::test::init()" << std::endl;
  // FieldSet
  const eckit::LocalConfiguration paramConfig(::test::TestEnvironment::config(), "parameters");
  const std::string gridName(paramConfig.getString("gridName"));
  const std::string partitionerType(paramConfig.getString("partitionerType"));
  const std::string meshType(paramConfig.getString("meshType"));

  // Initialise Atlas objects to produce FieldSet
  atlas::CubedSphereGrid grid(gridName);
  atlas::Mesh mesh(createMesh(grid, partitionerType, meshType));
  atlas::functionspace::CubedSphereNodeColumns functionSpace(createFunctionSpace(mesh));

  // VarMetadata
  const eckit::LocalConfiguration varMetadata = paramConfig.getSubConfiguration("varMetadata");
  for (const auto& key : varMetadata.keys()) {
    std::vector<std::string> stringVec = utils::strToWords(varMetadata.getString(key), ',');

    consts::FieldMetadata varMetadata;
    varMetadata.jediName = utils::strNoWhiteSpace(stringVec[consts::eJediName]);
    varMetadata.lfricReadName = utils::strNoWhiteSpace(stringVec[consts::eLfricReadName]);
    varMetadata.lfricWriteName = utils::strNoWhiteSpace(stringVec[consts::eLfricWriteName]);
    varMetadata.units = utils::strNoWhiteSpace(stringVec[consts::eUnits]);
    varMetadata.numberOfLevels =
                std::stoi(utils::strNoWhiteSpace(stringVec[consts::eNumberOfLevels]));
    varMetadata.copyFirstLevel = utils::strToBool(stringVec[consts::eCopyFirstLevel]);

    varMetadataVec.push_back(varMetadata);
  }
  fieldSet = createFieldSet(functionSpace, varMetadataVec);
  // Others
  dateTime = util::DateTime(paramConfig.getString("dateTime"));
  inputFilePath = paramConfig.getString("inputFilePath");
  outputFilePath = paramConfig.getString("outputFilePath");
}

void main() {
  atlas::FieldSet fieldSet;
  std::vector<consts::FieldMetadata> varMetadataVec;
  util::DateTime dateTime;
  std::string inputFilePath;
  std::string outputFilePath;
  initParams(fieldSet, varMetadataVec, dateTime, inputFilePath, outputFilePath);

  readInput(fieldSet, varMetadataVec, dateTime, inputFilePath);
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
