/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/AtlasData.h"

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
#include "oops/util/Logger.h"

namespace  {
atlas::Mesh createMesh(const atlas::CubedSphereGrid& grid,
                       const std::string& partitionerType,
                       const std::string& meshType)
{
  oops::Log::trace() << "createMesh()" << std::endl;
  const auto meshConfig = atlas::util::Config("partitioner", partitionerType) |
                          atlas::util::Config("halo", 0);
  const auto meshGen = atlas::MeshGenerator(meshType, meshConfig);
  return meshGen.generate(grid);
}

atlas::functionspace::CubedSphereNodeColumns createFunctionSpace(const atlas::Mesh& csMesh)
{
  oops::Log::trace() << "createFunctionSpace()" << std::endl;
  const auto functionSpace = atlas::functionspace::CubedSphereNodeColumns(csMesh);
  return functionSpace;
}

atlas::FieldSet createFieldSet(const atlas::functionspace::CubedSphereNodeColumns& functionSpace,
                               const std::map<std::string, std::tuple<std::string, int, size_t>>
                                                                               fieldToMetadataMap,
                               const bool isGlobal) {
  oops::Log::trace() << "createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& fieldMetadata : fieldToMetadataMap) {
    std::string atlasFieldName = std::get<monio::constants::eAtlasFieldName>(fieldMetadata.second);
    int dataType = std::get<monio::constants::eDataType>(fieldMetadata.second);
    size_t numLevels = std::get<monio::constants::eNumLevels>(fieldMetadata.second);
    atlas::util::Config atlasOptions = atlas::option::name(atlasFieldName) |
                                       atlas::option::levels(numLevels);
    if (isGlobal == true)
      atlasOptions = atlasOptions | atlas::option::global(0);
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
           const atlas::idx_t& mpiRankOwner,
           const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
           const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData,
           const std::string gridName,
           const std::string partitionerType,
           const std::string meshType):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner),
    atlasProcessor_(mpiCommunicator_, mpiRankOwner_),
    fieldToMetadataMap_(fieldToMetadataMap),
    grid_(gridName),
    mesh_(createMesh(grid_, partitionerType, meshType)),
    functionSpace_(createFunctionSpace(mesh_)) {
  oops::Log::trace() << "AtlasData::AtlasData()" << std::endl;
  atlasCoords_ = atlasProcessor_.getAtlasCoords(grid_);
  lfricCoords_ = atlasProcessor_.getLfricCoords(coordData);
  lfricAtlasMap_ = atlasProcessor_.createLfricAtlasMap(atlasCoords_, lfricCoords_);
}

void monio::AtlasData::initialiseMemberFieldSet() {
  localFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, false);
  globalFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, true);
}

void monio::AtlasData::toFieldSet(const Data& data) {
  oops::Log::trace() << "AtlasData::populateFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    const std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                                     data.getContainers();
    for (auto& fieldMetadata : fieldToMetadataMap_) {
      std::string lfricFieldName = fieldMetadata.first;
      std::string atlasFieldName =
               std::get<constants::eAtlasFieldName>(fieldMetadata.second);
      // Currently not used: int dataType = std::get<constants::eDataType>(fieldMetadata.second);
      size_t numLevels = std::get<constants::eNumLevels>(fieldMetadata.second);

      atlas::Field field = globalFieldSet_[atlasFieldName];
      field.set_levels(numLevels);
      const std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
      atlasProcessor_.populateFieldWithDataContainer(field, dataContainer, lfricAtlasMap_);
    }
  }
  scatterAtlasFields();
}

void monio::AtlasData::fromFieldSet(Data& data) {
  oops::Log::trace() << "AtlasData::fromFieldSet()" << std::endl;
  gatherAtlasFields();
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers
                                                                  = data.getContainers();
    for (auto& fieldMetadata : fieldToMetadataMap_) {
      std::string lfricFieldName = fieldMetadata.first;
      std::string atlasFieldName =
              std::get<constants::eAtlasFieldName>(fieldMetadata.second);
      size_t numLevels = std::get<constants::eNumLevels>(fieldMetadata.second);

      std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
      atlas::Field field = globalFieldSet_[atlasFieldName];
      field.set_levels(numLevels);
      atlasProcessor_.populateDataContainerWithField(dataContainer, field, lfricAtlasMap_);
    }
  }
}

void monio::AtlasData::scatterAtlasFields() {
  oops::Log::trace() << "AtlasData::scatterAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataMap_) {
    std::string atlasFieldName = std::get<constants::eAtlasFieldName>(fieldMetadata.second);
    functionSpace_.scatter(globalFieldSet_[atlasFieldName],
                           localFieldSet_[atlasFieldName]);
    localFieldSet_[atlasFieldName].haloExchange();
  }
}

void monio::AtlasData::gatherAtlasFields() {
  oops::Log::trace() << "AtlasData::gatherAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataMap_) {
    std::string atlasFieldName = std::get<constants::eAtlasFieldName>(fieldMetadata.second);
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
  oops::Log::trace() << "AtlasData::addField()" << std::endl;
  globalFieldSet_.add(field);
}
