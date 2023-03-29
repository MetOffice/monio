/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "AtlasData.h"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"

#include "atlas/grid/Iterator.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/util/KDTree.h"

namespace  {
atlas::Mesh createMesh(const atlas::CubedSphereGrid& grid,
                       const std::string& partitionerType,
                       const std::string& meshType)
{
  std::cout << "createMesh()" << std::endl;
  const auto meshConfig = atlas::util::Config("partitioner", partitionerType) |
                          atlas::util::Config("halo", 0);
  const auto meshGen = atlas::MeshGenerator(meshType, meshConfig);
  return meshGen.generate(grid);
}

atlas::functionspace::CubedSphereNodeColumns createFunctionSpace(const atlas::Mesh& csMesh)
{
  std::cout << "createFunctionSpace()" << std::endl;
  const auto functionSpace = atlas::functionspace::CubedSphereNodeColumns(csMesh);
  return functionSpace;
}

atlas::FieldSet createFieldSet(const atlas::functionspace::CubedSphereNodeColumns& functionSpace,
                               const std::vector<monio::constants::FieldMetadata>&
                                                                              fieldToMetadataVec,
                               const bool isGlobal) {
  std::cout << "createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& fieldMetadata : fieldToMetadataVec) {
    std::string atlasFieldName = fieldMetadata.atlasName;
    size_t numLevels = fieldMetadata.numLevels;
    int dataType = fieldMetadata.dataType;
    atlas::util::Config atlasOptions = atlas::option::name(atlasFieldName) |
                                       atlas::option::levels(numLevels);
    if (isGlobal == true) {
      atlasOptions = atlasOptions | atlas::option::global(0);
    }
    switch (dataType) {
      case monio::constants::eDataTypes::eDouble:
        fieldSet.add(functionSpace.createField<double>(atlasOptions));
        break;
      case monio::constants::eDataTypes::eFloat:
        fieldSet.add(functionSpace.createField<float>(atlasOptions));
        break;
      case monio::constants::eDataTypes::eInt:
        fieldSet.add(functionSpace.createField<int>(atlasOptions));
        break;
      default:
        throw std::runtime_error("createFieldSet()> Data type not coded for...");
    }
  }
  return fieldSet;
}
}  // anonymous namespace

monio::AtlasData::AtlasData(
           const eckit::mpi::Comm& mpiCommunicator,
           const atlas::idx_t mpiRankOwner,
           const std::vector<monio::constants::FieldMetadata> fieldToMetadataVec,
           const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData,
           const std::string gridName,
           const std::string partitionerType,
           const std::string meshType):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner),
    atlasProcessor_(mpiCommunicator_, mpiRankOwner_),
    fieldToMetadataVec_(fieldToMetadataVec),
    grid_(gridName),
    mesh_(createMesh(grid_, partitionerType, meshType)),
    functionSpace_(createFunctionSpace(mesh_)) {
  std::cout << "AtlasData::AtlasData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    atlasCoords_ = atlasProcessor_.getAtlasCoords(grid_);
    lfricCoords_ = atlasProcessor_.getLfricCoords(coordData);
    lfricAtlasMap_ = atlasProcessor_.createLfricAtlasMap(atlasCoords_, lfricCoords_);
  }
}

void monio::AtlasData::initialiseMemberFieldSet() {
  std::cout << "AtlasData::initialiseMemberFieldSet()" << std::endl;
  // This function needs to be called on all PEs - no MPI rank check!
  localFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataVec_, false);
  globalFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataVec_, true);
}

void monio::AtlasData::toFieldSet(const Data& data) {
  std::cout << "AtlasData::populateFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    const std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                                     data.getContainers();
    for (auto& fieldMetadata : fieldToMetadataVec_) {
      std::string lfricFieldName = fieldMetadata.lfricName;
      std::string atlasFieldName = fieldMetadata.atlasName;
      size_t numLevels = fieldMetadata.numLevels;
      // Not currently used: size_t fieldSize = fieldMetadata.fieldSize;

      atlas::Field field = globalFieldSet_[atlasFieldName];
      field.set_levels(numLevels);
      const std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
      atlasProcessor_.populateFieldWithDataContainer(field, dataContainer, lfricAtlasMap_);
    }
  }
  scatterAtlasFields();
}

void monio::AtlasData::fromFieldSet(Data& data) {
  std::cout << "AtlasData::fromFieldSet()" << std::endl;
  gatherAtlasFields();
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers
                                                                  = data.getContainers();
    for (auto& fieldMetadata : fieldToMetadataVec_) {
      std::string lfricFieldName = fieldMetadata.lfricName;
      std::string atlasFieldName = fieldMetadata.atlasName;
      size_t numLevels = fieldMetadata.numLevels;
      size_t fieldSize = fieldMetadata.fieldSize;

      std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
      atlas::Field field = globalFieldSet_[atlasFieldName];
      field.set_levels(numLevels);
      atlasProcessor_.populateDataContainerWithField(dataContainer, field,
                                                     lfricAtlasMap_, fieldSize);
    }
  }
}

void monio::AtlasData::scatterAtlasFields() {
  std::cout << "AtlasData::scatterAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataVec_) {
    std::string atlasFieldName = fieldMetadata.atlasName;
    functionSpace_.scatter(globalFieldSet_[atlasFieldName],
                           localFieldSet_[atlasFieldName]);
    localFieldSet_[atlasFieldName].haloExchange();
  }
}

void monio::AtlasData::gatherAtlasFields() {
  std::cout << "AtlasData::gatherAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataVec_) {
    std::string atlasFieldName = fieldMetadata.atlasName;
    localFieldSet_[atlasFieldName].haloExchange();
    functionSpace_.gather(localFieldSet_[atlasFieldName],
                          globalFieldSet_[atlasFieldName]);
  }
}

atlas::FieldSet& monio::AtlasData::getGlobalFieldSet() {
  return globalFieldSet_;
}

atlas::FieldSet& monio::AtlasData::getLocalFieldSet() {
  return localFieldSet_;
}

void monio::AtlasData::addField(atlas::Field& field) {
  std::cout << "AtlasData::addField()" << std::endl;
  globalFieldSet_.add(field);
}
