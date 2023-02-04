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
#include "Metadata.h"

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

atlas::Field createField(const atlas::functionspace::CubedSphereNodeColumns& functionSpace,
                         const std::string& fieldName,
                         const bool isGlobal) {
  oops::Log::trace() << "createField()" << std::endl;
  atlas::util::Config atlasOptions = atlas::option::name(fieldName);
  if (isGlobal == true)
    atlasOptions = atlas::option::name(fieldName) |
                   atlas::option::global(monio::constants::kMPIRankOwner);
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

monio::AtlasData::AtlasData(
           const eckit::mpi::Comm& mpiCommunicator,
           const atlas::idx_t& mpiRankOwner,
           const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& coordDataMap,
           const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
           const std::string gridName,
           const std::string partitionerType,
           const std::string meshType):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner),
    fieldToMetadataMap_(fieldToMetadataMap),
    grid_(gridName),
    mesh_(createMesh(grid_, partitionerType, meshType)),
    functionSpace_(createFunctionSpace(mesh_)),
    pointsLonLat_(initPointsLonLat(grid_)),
    lfricCoordData_(processLfricCoordData(coordDataMap)),
    lfricToAtlasMap_(createLfricToAtlasMap(pointsLonLat_, lfricCoordData_)) {
  oops::Log::trace() << "AtlasData::AtlasData()" << std::endl;
}

monio::AtlasData::AtlasData(const eckit::mpi::Comm& mpiCommunicator,
                            const atlas::idx_t& mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "AtlasData::AtlasData()" << std::endl;
}

monio::AtlasData::~AtlasData() {}

void monio::AtlasData::initialiseNewFieldSet() {
  localFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, false);
  globalFieldSet_ = createFieldSet(functionSpace_, fieldToMetadataMap_, true);
}

void monio::AtlasData::setLfricToAtlasMap(const std::vector<size_t>& lfricToAtlasMap) {
  lfricToAtlasMap_ = lfricToAtlasMap;
}

void monio::AtlasData::toFieldSet(const Data& data) {
  oops::Log::trace() << "AtlasData::populateFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    toAtlasFields(data);
  }
  scatterAtlasFields();
}

void monio::AtlasData::fromFieldSet(Data& data) {
  oops::Log::trace() << "AtlasData::fromFieldSet()" << std::endl;
  gatherAtlasFields();
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    fromAtlasFields(data);
  }
}

void monio::AtlasData::toAtlasFields(const Data& data) {
  oops::Log::trace() << "AtlasData::toAtlasFields()" << std::endl;
  const std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers =
                                                                   data.getContainers();
  for (auto& fieldMetadata : fieldToMetadataMap_) {
    std::string lfricFieldName = fieldMetadata.first;
    std::string atlasFieldName =
             std::get<constants::eAtlasFieldName>(fieldMetadata.second);
    int dataType = std::get<constants::eDataType>(fieldMetadata.second);
    size_t numLevels = std::get<constants::eNumLevels>(fieldMetadata.second);

    const std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
    switch (dataType) {
    case constants::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
      toAtlasField(atlasFieldName, numLevels, dataContainerDouble->getData());
      break;
    }
    case constants::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
      toAtlasField(atlasFieldName, numLevels, dataContainerFloat->getData());
      break;
    }
    case constants::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
      toAtlasField(atlasFieldName, numLevels, dataContainerInt->getData());
      break;
    }
    default:
      throw std::runtime_error("AtlasData::toAtlasFields()> Data type not coded for...");
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

void monio::AtlasData::fromAtlasFields(Data& data) {
  oops::Log::trace() << "AtlasData::fromAtlasFields()" << std::endl;
  std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainers = data.getContainers();

  for (auto& fieldMetadata : fieldToMetadataMap_) {
    std::string lfricFieldName = fieldMetadata.first;
    std::string atlasFieldName =
            std::get<constants::eAtlasFieldName>(fieldMetadata.second);
    size_t numLevels = std::get<constants::eNumLevels>(fieldMetadata.second);

    std::shared_ptr<DataContainerBase> dataContainer = dataContainers.at(lfricFieldName);
    atlas::Field field = localFieldSet_[atlasFieldName];
    atlas::array::DataType atlasType = field.datatype();

    switch (atlasType.kind()) {
    case atlasType.KIND_INT32: {
      std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
      dataContainerInt->resetData();
      fromAtlasField(atlasFieldName, numLevels, dataContainerInt->getData());
      break;
    }
    case atlasType.KIND_REAL32: {
      std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
      dataContainerFloat->resetData();
      fromAtlasField(atlasFieldName, numLevels, dataContainerFloat->getData());
      break;
    }
    case atlasType.KIND_REAL64: {
      std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
      dataContainerDouble->resetData();
      fromAtlasField(atlasFieldName, numLevels, dataContainerDouble->getData());
      break;
    }
    default:
      throw std::runtime_error("AtlasData::fromAtlasFields()> Data type not coded for...");
    }
  }
}

template<typename T>
void monio::AtlasData::toAtlasField(const std::string& atlasFieldName,
                                    const int numLevels,
                                    const std::vector<T>& dataVec) {
  oops::Log::trace() << "AtlasData::fieldToAtlas()" << std::endl;

  auto atlasField = globalFieldSet_[atlasFieldName];
  auto fieldView = atlas::array::make_view<T, 2>(atlasField);

  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasData::toAtlasField<double>(const std::string& atlasFieldName,
                                                     const int numLevels,
                                                     const std::vector<double>& dataVec);
template void monio::AtlasData::toAtlasField<float>(const std::string& atlasFieldName,
                                                    const int numLevels,
                                                    const std::vector<float>& dataVec);
template void monio::AtlasData::toAtlasField<int>(const std::string& atlasFieldName,
                                                  const int numLevels,
                                                  const std::vector<int>& dataVec);

template<typename T>
void monio::AtlasData::fromAtlasField(const std::string& atlasFieldName,
                                      const int numLevels,
                                      std::vector<T>& dataVec) {
  oops::Log::trace() << "AtlasData::atlasToField()" << std::endl;
  auto atlasField = globalFieldSet_[atlasFieldName];
  populateField(atlasField, numLevels, dataVec);
}

template void monio::AtlasData::fromAtlasField<double>(const std::string& atlasFieldName,
                                                       const int numLevels,
                                                       std::vector<double>& dataVec);
template void monio::AtlasData::fromAtlasField<float>(const std::string& atlasFieldName,
                                                      const int numLevels,
                                                      std::vector<float>& dataVec);
template void monio::AtlasData::fromAtlasField<int>(const std::string& atlasFieldName,
                                                    const int numLevels,
                                                    std::vector<int>& dataVec);

template<typename T>
void monio::AtlasData::populateField(atlas::Field& field,
                                     const int numLevels,
                                     const std::vector<T>& dataVec) {
  oops::Log::trace() << "AtlasData::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasData::populateField<double>(atlas::Field& field,
                                                      const int numLevels,
                                                      const std::vector<double>& dataVec);
template void monio::AtlasData::populateField<float>(atlas::Field& field,
                                                     const int numLevels,
                                                     const std::vector<float>& dataVec);
template void monio::AtlasData::populateField<int>(atlas::Field& field,
                                                   const int numLevels,
                                                   const std::vector<int>& dataVec);

atlas::FieldSet& monio::AtlasData::getGlobalFieldSet() {
  return globalFieldSet_;
}

atlas::FieldSet& monio::AtlasData::getLocalFieldSet() {
  return localFieldSet_;
}

std::vector<atlas::PointLonLat> monio::AtlasData::processLfricCoordData(
                    const std::map<std::string,
                    std::shared_ptr<monio::DataContainerBase>>& coordDataMap) {
  oops::Log::trace() << "AtlasData::processLfricCoordData()" << std::endl;
  std::vector<atlas::PointLonLat> lfricLonLat;
  if (coordDataMap.size() <= 2) {
    std::array<std::vector<float>, 2> coordVectorArray;
    int coordCount = 0;
    for (auto& coordDataPair : coordDataMap) {
      std::shared_ptr<DataContainerBase> coordContainer = coordDataPair.second;
      // LFRic coordinate data are currently stored as floats
      if (coordContainer->getType() == constants::eFloat) {
        std::shared_ptr<DataContainerFloat> cooordContainerFloat =
                  std::static_pointer_cast<DataContainerFloat>(coordContainer);
        std::vector<float> coordData = cooordContainerFloat->getData();

        // Essential check to ensure grid is configured to accommodate the data
        if (coordData.size() != grid_.size()) {
          throw std::runtime_error("AtlasData::processLfricCoordData()> "
              "Configured grid is not compatible with input file...");
        }
        coordVectorArray[coordCount] = coordData;
        coordCount++;
      } else {
        throw std::runtime_error("AtlasData::processLfricCoordData()> "
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
      throw std::runtime_error("AtlasData::processLfricCoordData()> "
          "More than 2 coordinate axis...");
  }
  return lfricLonLat;
}

void monio::AtlasData::addField(atlas::Field& field) {
  oops::Log::trace() << "AtlasData::addField()" << std::endl;
  globalFieldSet_.add(field);
}

void monio::AtlasData::populateFieldWithData(
                                 atlas::Field& field,
                                 const int numLevels,
                                 const std::shared_ptr<monio::DataContainerBase>& dataContainer) {
  oops::Log::trace() << "AtlasData::populateFieldWithData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case constants::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, numLevels, dataContainerDouble->getData());
      break;
    }
    case constants::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, numLevels, dataContainerFloat->getData());
      break;
    }
    case constants::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, numLevels, dataContainerInt->getData());
      break;
    }
    default:
      throw std::runtime_error("AtlasData::toAtlasFields()> Data type not coded for...");
    }
  }
}
