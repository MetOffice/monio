/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/AtlasProcessor.h"

#include <algorithm>
#include <numeric>

#include "atlas/grid/Iterator.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/util/KDTree.h"
#include "oops/util/Logger.h"

monio::AtlasProcessor::AtlasProcessor(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t& mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "AtlasProcessor::AtlasProcessor()" << std::endl;
}

void monio::AtlasProcessor::initAtlasLonLat(atlas::CubedSphereGrid& grid) {
  oops::Log::trace() << "initPointsLonLat()" << std::endl;
  atlasCoords_.resize(grid.size());
  std::copy(grid.lonlat().begin(), grid.lonlat().end(), atlasCoords_.begin());
}

void monio::AtlasProcessor::populateFieldWithDataContainer(atlas::Field& field,
                                 const std::shared_ptr<DataContainerBase>& dataContainer) {
  oops::Log::trace() << "AtlasData::populateFieldWithData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case constants::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData());
      break;
    }
    case constants::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData());
      break;
    }
    case constants::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData());
      break;
    }
    default:
      throw std::runtime_error("AtlasData::toAtlasFields()> Data type not coded for...");
    }
  }
}

template<typename T>
void monio::AtlasProcessor::populateField(atlas::Field& field, const std::vector<T>& dataVec) {
  oops::Log::trace() << "AtlasProcessor::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  size_t numLevels = field.levels();
  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasProcessor::populateField<double>(atlas::Field& field,
                                                           const std::vector<double>& dataVec);
template void monio::AtlasProcessor::populateField<float>(atlas::Field& field,
                                                          const std::vector<float>& dataVec);
template void monio::AtlasProcessor::populateField<int>(atlas::Field& field,
                                                        const std::vector<int>& dataVec);
void monio::AtlasProcessor::populateDataWithField(Data& data, const atlas::Field field) {
    std::string fieldName = field.name();

    size_t numLevels = field.levels();
    const auto lonLat = field.functionspace()->lonlat();

    std::shared_ptr<DataContainerBase> dataContainer = nullptr;
    atlas::array::DataType dataType = field.datatype();

    switch (dataType.kind()) {
      case dataType.KIND_INT32: {
        std::shared_ptr<DataContainerInt> dataContainerInt = std::make_shared<DataContainerInt>(fieldName);
        dataContainerInt->setSize(field.size());
        populateDataVec(dataContainerInt->getData(), field);
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerInt);
        break;
      }
      case dataType.KIND_REAL32: {
        std::shared_ptr<DataContainerFloat> dataContainerFloat = std::make_shared<DataContainerFloat>(fieldName);
        populateDataVec(dataContainerFloat->getData(), field);
        dataContainerFloat->setSize(field.size());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerFloat);
        //std::shared_ptr<DataContainerFloat> dataContainerFloat = std::static_pointer_cast<DataContainerFloat>(dataContainer);
        //fromAtlasField(atlasFieldName, numLevels, dataContainerFloat->getData());
        //dataContainerFloat->setSize(field.size());
        break;
      }
      case dataType.KIND_REAL64: {
        std::shared_ptr<DataContainerDouble> dataContainerDouble = std::make_shared<DataContainerDouble>(fieldName);
        populateDataVec(dataContainerDouble->getData(), field);
        dataContainerDouble->setSize(field.size());
        dataContainer = std::static_pointer_cast<DataContainerBase>(dataContainerDouble);
        //std::shared_ptr<DataContainerDouble> dataContainerDouble = std::static_pointer_cast<DataContainerDouble>(dataContainer);
        //fromAtlasField(atlasFieldName, numLevels, dataContainerDouble->getData());
        //dataContainerDouble->setSize(field.size());
        break;
      }
    }
    data.addContainer(dataContainer);
}

void monio::AtlasProcessor::populateDataWithFieldSet(Data& data, const atlas::FieldSet& fieldSet) {
  for(const auto& field : fieldSet) {
    populateDataWithField(data, field);
  }
}

void monio::AtlasProcessor::processLfricCoordData(
      const std::map<std::string, std::shared_ptr<DataContainerBase>>& coordDataMap,
      const std::vector<atlas::PointLonLat>& atlasCoords) {
  oops::Log::trace() << "AtlasData::processLfricCoordData()" << std::endl;
  atlasCoords_ = atlasCoords;
  lfricCoords_.clear();
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
//        if (coordData.size() != grid_.size()) {
//          throw std::runtime_error("AtlasData::processLfricCoordData()> "
//              "Configured grid is not compatible with input file...");
//        }
        coordVectorArray[coordCount] = coordData;
        coordCount++;
      } else {
        throw std::runtime_error("AtlasData::processLfricCoordData()> "
            "Data type not coded for...");
      }
    }
    // Populate Atlas PointLonLat vector
    lfricCoords_.reserve(coordVectorArray[0].size());
    for (auto lonIt = coordVectorArray[0].begin(), latIt = coordVectorArray[1].begin();
                                  lonIt != coordVectorArray[0].end(); ++lonIt , ++latIt) {
      lfricCoords_.push_back(atlas::PointLonLat(*lonIt, *latIt));
    }
    createLfricToAtlasMap(atlasCoords_, lfricCoords_);
  } else {
      throw std::runtime_error("AtlasData::processLfricCoordData()> "
          "More than 2 coordinate axis...");
  }
}

void monio::AtlasProcessor::setLfricToAtlasMap(const std::vector<size_t>& lfricToAtlasMap) {
  lfricToAtlasMap_ = lfricToAtlasMap;
}

void monio::AtlasProcessor::createLfricToAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                                  const std::vector<atlas::PointLonLat>& lfricCoords) {
  oops::Log::trace() << "AtlasProcessor::createLfricToAtlasMap()" << std::endl;
  lfricToAtlasMap_.clear();
  lfricToAtlasMap_.reserve(atlasCoords.size());

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
    lfricToAtlasMap_.push_back(idx);
  }
}

template<typename T>
void monio::AtlasProcessor::populateDataVec(std::vector<T>& dataVec,
                                            const atlas::Field& field) {
  oops::Log::trace() << "AtlasData::populateDataVec()" << std::endl;
  size_t numLevels = field.levels();
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (size_t i = 0; i < lfricToAtlasMap_.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap_[i] + (j * lfricToAtlasMap_.size());
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasProcessor::populateDataVec<double>(std::vector<double>& dataVec,
                                                             const atlas::Field& field);
template void monio::AtlasProcessor::populateDataVec<float>(std::vector<float>& dataVec,
                                                            const atlas::Field& field);
template void monio::AtlasProcessor::populateDataVec<int>(std::vector<int>& dataVec,
                                                          const atlas::Field& field);
