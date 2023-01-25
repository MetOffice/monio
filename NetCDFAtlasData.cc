/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFAtlasData.h"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>

#include "NetCDFConstants.h"
#include "NetCDFDataContainerDouble.h"
#include "NetCDFDataContainerFloat.h"
#include "NetCDFDataContainerInt.h"
#include "NetCDFMetadata.h"

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
                               std::map<std::string, std::tuple<std::string, int, size_t>>
                                                                               fieldToMetadataMap,
                               const bool isGlobal) {
  oops::Log::trace() << "createFieldSet()" << std::endl;
  atlas::FieldSet fieldSet;
  for (const auto& fieldMetadata : fieldToMetadataMap) {
    std::string atlasFieldName =
            std::get<lfriclite::ncconsts::eAtlasFieldName>(fieldMetadata.second);
    int dataType = std::get<lfriclite::ncconsts::eDataType>(fieldMetadata.second);
    size_t numLevels = std::get<lfriclite::ncconsts::eNumLevels>(fieldMetadata.second);
    atlas::util::Config atlasOptions = atlas::option::name(atlasFieldName) |
                                       atlas::option::levels(numLevels);
    if (isGlobal == true)
      atlasOptions = atlasOptions | atlas::option::global(0);
    switch (dataType) {
      case lfriclite::ncconsts::dataTypesEnum::eDouble:
        fieldSet.add(functionSpace.createField<double>(atlasOptions));
        break;
      case lfriclite::ncconsts::dataTypesEnum::eFloat:
        fieldSet.add(functionSpace.createField<float>(atlasOptions));
        break;
      case lfriclite::ncconsts::dataTypesEnum::eInt:
        fieldSet.add(functionSpace.createField<int>(atlasOptions));
        break;
      default:
        throw std::runtime_error("createFieldSet()> Data type not coded for...");
    }
  }
  return fieldSet;
}

atlas::Field createField(const atlas::functionspace::CubedSphereNodeColumns& functionSpace,
                               const std::string& fieldName,
                               const bool isGlobal) {
  oops::Log::trace() << "createField()" << std::endl;
  atlas::util::Config atlasOptions = atlas::option::name(fieldName);
  if (isGlobal == true)
    atlasOptions = atlas::option::name(fieldName) |
                   atlas::option::global(lfriclite::ncconsts::kMPIRankOwner);
  // Currently hard-coded to create double precision fields. Type info stored in meta/data.
  atlas::Field field = functionSpace.createField<double>(atlasOptions);

  return field;
}

const std::vector<atlas::PointLonLat> initPointsLonLat(atlas::CubedSphereGrid& grid) {
  oops::Log::trace() << "initPointsLonLat()" << std::endl;
  std::vector<atlas::PointLonLat> pointsLonLat(grid.size());
  std::copy(grid.lonlat().begin(), grid.lonlat().end(), pointsLonLat.begin());

  return pointsLonLat;
}

std::vector<size_t> createLfricToAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                          const std::vector<atlas::PointLonLat>& lfricCoords) {
  oops::Log::trace() << "createLfricToAtlasMap()" << std::endl;
  std::vector<size_t> coordMap;
  coordMap.reserve(atlasCoords.size());

  // Make a kd-tree using atlasLonLat as the point,
  // with element index i as payload
  std::vector<size_t> indices(atlasCoords.size());
  std::iota(begin(indices), end(indices), 0);

  const atlas::Geometry unitSphere(1.0);
  atlas::util::IndexKDTree tree(unitSphere);
  tree.build(atlasCoords, indices);

  // Partitioning vector to create atlas::Distribution for mesh generation
  std::vector<int> partitioning(tree.size(), -1);

  // find atlas global indices for each element of modelLonLat
  for (const auto& lfricCoord : lfricCoords) {
    auto idx = tree.closestPoint(lfricCoord).payload();
    coordMap.push_back(idx);
  }
  return coordMap;
}
}  // anonymous namespace

lfriclite::NetCDFAtlasData::NetCDFAtlasData(
           const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap,
           const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
           const std::string gridName,
           const std::string partitionerType,
           const std::string meshType):
    fieldToMetadataMap_(fieldToMetadataMap),
    grid_(gridName),
    mesh_(createMesh(grid_, partitionerType, meshType)),
    functionSpace_(createFunctionSpace(mesh_)),
    pointsLonLat_(initPointsLonLat(grid_)),
    lfricCoordData_(processLfricCoordData(coordDataMap)),
    lfricToAtlasMap_(createLfricToAtlasMap(pointsLonLat_, lfricCoordData_)) {
  oops::Log::trace() << "NetCDFAtlasData::NetCDFAtlasData()" << std::endl;
  fieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, false);
  globalFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, true);
}

lfriclite::NetCDFAtlasData::~NetCDFAtlasData() {}

void lfriclite::NetCDFAtlasData::toAtlasFields(NetCDFData* data) {
  oops::Log::trace() << "NetCDFAtlasData::toAtlasFields()" << std::endl;
  std::map<std::string, NetCDFDataContainerBase*>& dataContainers = data->getContainers();
  for (auto& fieldMetadata : fieldToMetadataMap_) {
    std::string lfricFieldName = fieldMetadata.first;
    std::string atlasFieldName =
            std::get<lfriclite::ncconsts::eAtlasFieldName>(fieldMetadata.second);
    int dataType = std::get<lfriclite::ncconsts::eDataType>(fieldMetadata.second);
    size_t numLevels = std::get<lfriclite::ncconsts::eNumLevels>(fieldMetadata.second);

    NetCDFDataContainerBase* dataContainer = dataContainers[lfricFieldName];
    switch (dataType) {
    case lfriclite::ncconsts::dataTypesEnum::eDouble: {
      NetCDFDataContainerDouble* dataContainerDouble =
          static_cast<NetCDFDataContainerDouble*>(dataContainer);
      fieldToAtlas(atlasFieldName, numLevels, dataContainerDouble->getData());
      break;
    }
    case lfriclite::ncconsts::dataTypesEnum::eFloat: {
      NetCDFDataContainerFloat* dataContainerFloat =
          static_cast<NetCDFDataContainerFloat*>(dataContainer);
      fieldToAtlas(atlasFieldName, numLevels, dataContainerFloat->getData());
      break;
    }
    case lfriclite::ncconsts::dataTypesEnum::eInt: {
      NetCDFDataContainerInt* dataContainerInt =
          static_cast<NetCDFDataContainerInt*>(dataContainer);
      fieldToAtlas(atlasFieldName, numLevels, dataContainerInt->getData());
      break;
    }
    default:
      throw std::runtime_error("NetCDFAtlasData::toAtlasFields()> Data type not coded for...");
    }
  }
}

void lfriclite::NetCDFAtlasData::scatterAtlasFields() {
  oops::Log::trace() << "NetCDFAtlasData::scatterAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataMap_) {
    std::string atlasFieldName =
            std::get<lfriclite::ncconsts::eAtlasFieldName>(fieldMetadata.second);
    functionSpace_.scatter(globalFieldSet_[atlasFieldName], fieldSet_[atlasFieldName]);
    fieldSet_[atlasFieldName].haloExchange();
  }
}

void lfriclite::NetCDFAtlasData::gatherAtlasFields() {
  oops::Log::trace() << "NetCDFAtlasData::gatherAtlasFields()" << std::endl;
  for (const auto& fieldMetadata : fieldToMetadataMap_) {
    std::string atlasFieldName =
            std::get<lfriclite::ncconsts::eAtlasFieldName>(fieldMetadata.second);
    fieldSet_[atlasFieldName].haloExchange();
    functionSpace_.gather(fieldSet_[atlasFieldName], globalFieldSet_[atlasFieldName]);
  }
}

void lfriclite::NetCDFAtlasData::fromAtlasFields(NetCDFData* data) {
  oops::Log::trace() << "NetCDFAtlasData::fromAtlasFields()" << std::endl;
  std::map<std::string, NetCDFDataContainerBase*>& dataContainers = data->getContainers();

  for (auto& fieldMetadata : fieldToMetadataMap_) {
    std::string lfricFieldName = fieldMetadata.first;
    std::string atlasFieldName =
            std::get<lfriclite::ncconsts::eAtlasFieldName>(fieldMetadata.second);
    size_t numLevels = std::get<lfriclite::ncconsts::eNumLevels>(fieldMetadata.second);

    NetCDFDataContainerBase* dataContainer = dataContainers[lfricFieldName];
    atlas::Field field = fieldSet_[atlasFieldName];
    atlas::array::DataType atlasType = field.datatype();

    switch (atlasType.kind()) {
    case atlasType.KIND_INT32: {
      NetCDFDataContainerInt* dataContainerInt =
          static_cast<NetCDFDataContainerInt*>(dataContainer);
      dataContainerInt->resetData();
      atlasToField(atlasFieldName, numLevels, dataContainerInt->getData());
      break;
    }
    case atlasType.KIND_REAL32: {
      NetCDFDataContainerFloat* dataContainerFloat =
          static_cast<NetCDFDataContainerFloat*>(dataContainer);
      dataContainerFloat->resetData();
      atlasToField(atlasFieldName, numLevels, dataContainerFloat->getData());
      break;
    }
    case atlasType.KIND_REAL64: {
      NetCDFDataContainerDouble* dataContainerDouble =
          static_cast<NetCDFDataContainerDouble*>(dataContainer);
      dataContainerDouble->resetData();
      atlasToField(atlasFieldName, numLevels, dataContainerDouble->getData());
      break;
    }
    default:
      throw std::runtime_error("NetCDFAtlasData::fromAtlasFields()> Data type not coded for...");
    }
  }
}

template<typename T>
void lfriclite::NetCDFAtlasData::fieldToAtlas(const std::string& atlasFieldName,
                                              const int& numLevels,
                                              const std::vector<T>& dataVec) {
  oops::Log::trace() << "NetCDFAtlasData::fieldToAtlas()" << std::endl;

  auto atlasField = globalFieldSet_[atlasFieldName];
  auto fieldView = atlas::array::make_view<T, 2>(atlasField);

  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void lfriclite::NetCDFAtlasData::fieldToAtlas<double>(const std::string& atlasFieldName,
                                                               const int& numLevels,
                                                               const std::vector<double>& dataVec);
template void lfriclite::NetCDFAtlasData::fieldToAtlas<float>(const std::string& atlasFieldName,
                                                              const int& numLevels,
                                                              const std::vector<float>& dataVec);
template void lfriclite::NetCDFAtlasData::fieldToAtlas<int>(const std::string& atlasFieldName,
                                                            const int& numLevels,
                                                            const std::vector<int>& dataVec);

template<typename T>
void lfriclite::NetCDFAtlasData::atlasToField(const std::string& atlasFieldName,
                                              const int& numLevels,
                                              std::vector<T>& dataVec) {
  oops::Log::trace() << "NetCDFAtlasData::atlasToField()" << std::endl;

  auto atlasField = globalFieldSet_[atlasFieldName];
  auto fieldView = atlas::array::make_view<T, 2>(atlasField);

  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void lfriclite::NetCDFAtlasData::atlasToField<double>(const std::string& atlasFieldName,
                                                               const int& numLevels,
                                                               std::vector<double>& dataVec);
template void lfriclite::NetCDFAtlasData::atlasToField<float>(const std::string& atlasFieldName,
                                                              const int& numLevels,
                                                              std::vector<float>& dataVec);
template void lfriclite::NetCDFAtlasData::atlasToField<int>(const std::string& atlasFieldName,
                                                            const int& numLevels,
                                                            std::vector<int>& dataVec);

atlas::FieldSet lfriclite::NetCDFAtlasData::getGlobalFieldSet() {
  return globalFieldSet_;
}

atlas::FieldSet lfriclite::NetCDFAtlasData::getFieldSet() {
  return fieldSet_;
}

std::vector<atlas::PointLonLat> lfriclite::NetCDFAtlasData::processLfricCoordData(
                        const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap) {
  oops::Log::trace() << "NetCDFAtlasData::processLfricCoordData()" << std::endl;
  std::vector<atlas::PointLonLat> lfricLonLat;
  if (coordDataMap.size() <= 2) {
    std::array<std::vector<float>, 2> coordVectorArray;
    int coordCount = 0;
    for (auto& coordDataPair : coordDataMap) {
      NetCDFDataContainerBase* coordContainer = coordDataPair.second;
      // LFRic coordinate data are currently stored as floats
      if (coordContainer->getType() == lfriclite::ncconsts::eFloat) {
        NetCDFDataContainerFloat* cooordContainerFloat =
                  static_cast<NetCDFDataContainerFloat*>(coordContainer);
        std::vector<float> coordData = cooordContainerFloat->getData();

        // Essential check to ensure grid is configured to accommodate the data
        if (coordData.size() != grid_.size()) {
          throw std::runtime_error("NetCDFAtlasData::processLfricCoordData()> "
              "Configured grid is not compatible with input file...");
        }
        coordVectorArray[coordCount] = coordData;
        coordCount++;
      } else {
        throw std::runtime_error("NetCDFAtlasData::processLfricCoordData()> "
            "Data type not coded for...");
      }
    }
    // Populate Atlas PointLonLat vector
    lfricLonLat.reserve(coordVectorArray[0].size());
    for (auto lonIt = coordVectorArray[0].begin(), latIt = coordVectorArray[1].begin();
                                  lonIt != coordVectorArray[0].end(); ++lonIt , ++latIt) {
      lfricLonLat.push_back(atlas::PointLonLat(*lonIt, *latIt));
    }
  } else {
      throw std::runtime_error("NetCDFAtlasData::processLfricCoordData()> "
          "More than 2 coordinate axis...");
  }
  return lfricLonLat;
}

void lfriclite::NetCDFAtlasData::addField(atlas::Field field) {
  oops::Log::trace() << "NetCDFAtlasData::addField()" << std::endl;
  globalFieldSet_.add(field);
}
