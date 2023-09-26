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
                               std::vector<consts::FieldMetadata>& fieldMetadataVec) {
  oops::Log::debug() << "monio::test::createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& fieldMetadata : fieldMetadataVec) {
    // No error checking on metadata. This is handled by calls to Monio
    atlas::util::Config atlasOptions = atlas::option::name(fieldMetadata.jediName) |
                                       atlas::option::levels(fieldMetadata.numberOfLevels);
    fieldSet.add(functionSpace.createField<double>(atlasOptions));
  }
  return fieldSet;
}

/// Writes FieldSet to file.
void write(const atlas::FieldSet& fieldSet, const std::string& outputFilePath) {
  oops::Log::info() << "monio::test::write()" << std::endl;
  // No further formatting required. Write directly to file
  Monio::get().writeFieldSet(fieldSet, outputFilePath);
}

/// Reads data from file and populates the FieldSet
void readInput(atlas::FieldSet& fieldSet,
               const std::vector<consts::FieldMetadata>& fieldMetadataVec,
               const util::DateTime& dateTime,
               const std::string& inputFilePath) {
  oops::Log::info() << "monio::test::readInput()" << std::endl;
  oops::Log::info() << "inputFilePath> " << inputFilePath << std::endl;
  oops::Log::info() << "dateTime> " << dateTime << std::endl;

  Monio::get().readState(fieldSet, fieldMetadataVec, inputFilePath, dateTime);
}

/// Sets up the objects required to mimic an operational call to Monio::Read via readInput
void initParams(atlas::FieldSet& fieldSet,
                std::vector<consts::FieldMetadata>& fieldMetadataVec,
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

  // fieldMetadata
  const eckit::LocalConfiguration fieldMetadata = paramConfig.getSubConfiguration("fieldMetadata");
  for (const auto& key : fieldMetadata.keys()) {
    std::vector<std::string> stringVec = utils::strToWords(fieldMetadata.getString(key), ',');

    consts::FieldMetadata fieldMetadata;
    fieldMetadata.jediName = utils::strNoWhiteSpace(stringVec[consts::eJediName]);
    fieldMetadata.lfricReadName = utils::strNoWhiteSpace(stringVec[consts::eLfricReadName]);
    fieldMetadata.lfricWriteName = utils::strNoWhiteSpace(stringVec[consts::eLfricWriteName]);
    fieldMetadata.units = utils::strNoWhiteSpace(stringVec[consts::eUnits]);
    fieldMetadata.numberOfLevels =
                    std::stoi(utils::strNoWhiteSpace(stringVec[consts::eNumberOfLevels]));
    fieldMetadata.noFirstLevel = utils::strToBool(stringVec[consts::eNoFirstLevel]);

    fieldMetadataVec.push_back(fieldMetadata);
  }
  fieldSet = createFieldSet(functionSpace, fieldMetadataVec);
  // Others
  dateTime = util::DateTime(paramConfig.getString("dateTime"));
  inputFilePath = paramConfig.getString("inputFilePath");
  outputFilePath = paramConfig.getString("outputFilePath");
}

void main() {
  atlas::FieldSet fieldSet;
  std::vector<consts::FieldMetadata> fieldMetadataVec;
  util::DateTime dateTime;
  std::string inputFilePath;
  std::string outputFilePath;

  initParams(fieldSet, fieldMetadataVec, dateTime, inputFilePath, outputFilePath);
  readInput(fieldSet, fieldMetadataVec, dateTime, inputFilePath);
  write(fieldSet, outputFilePath);
}

class FieldSetWrite : public oops::Test{
 public:
  FieldSetWrite() {}
  virtual ~FieldSetWrite() {}

 private:
  std::string testid() const override {
    return "monio::test::FieldSetWrite";
  }

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    std::function<void(std::string&, int&, int)> mainFunction =
        [&](std::string&, int&, int) { main(); };
    ts.push_back(eckit::testing::Test("monio/test_fieldset_write", mainFunction));
  }
  void clear() const override {}
};
}  // namespace test
}  // namespace monio
