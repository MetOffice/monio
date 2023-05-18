/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "AtlasProcessor.h"

#include <algorithm>
#include <numeric>

#include "atlas/functionspace.h"
#include "atlas/grid/Iterator.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/util/KDTree.h"
#include "oops/util/Logger.h"

#include "AttributeString.h"
#include "Metadata.h"
#include "Writer.h"

monio::AtlasProcessor::AtlasProcessor(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasProcessor::AtlasProcessor()" << std::endl;
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getLfricCoords(
                     const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData) {
  oops::Log::debug() << "AtlasProcessor::getLfricCoords()" << std::endl;

  std::vector<atlas::PointLonLat> lfricCoords;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (coordData.size() == 2) {
      std::array<std::vector<float>, 2> coordVectorArray;
      int coordCount = 0;
      for (auto& coordContainer : coordData) {
        // LFRic coordinate data are currently stored as floats
        if (coordContainer->getType() == constants::eFloat) {
          std::shared_ptr<DataContainerFloat> cooordContainerFloat =
                    std::static_pointer_cast<DataContainerFloat>(coordContainer);
          std::vector<float> coordData = cooordContainerFloat->getData();
          coordVectorArray[coordCount] = coordData;
          coordCount++;
        } else {
          throw std::runtime_error("AtlasProcessor::getLfricCoords()> "
              "Data type not coded for...");
        }
      }
      // Populate Atlas PointLonLat vector
      lfricCoords.reserve(coordVectorArray[0].size());
      for (auto lonIt = coordVectorArray[0].begin(), latIt = coordVectorArray[1].begin();
                                    lonIt != coordVectorArray[0].end(); ++lonIt , ++latIt) {
        lfricCoords.push_back(atlas::PointLonLat(*lonIt, *latIt));
      }
    } else {
        throw std::runtime_error("AtlasProcessor::getLfricCoords()> "
            "Incorrect number of coordinate axes...");
    }
  }
  return lfricCoords;
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getAtlasCoords(
                                                           const atlas::Field& field) {
  oops::Log::debug() << "AtlasProcessor::getAtlasCoords()" << std::endl;
  std::vector<atlas::PointLonLat> atlasCoords;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (field.metadata().get<bool>("global") == false) {
      auto lonLatField = field.functionspace().lonlat();
      auto lonLatView = atlas::array::make_view<double, 2>(lonLatField);
      for (int i = 0; i < getSizeOwned(field); ++i) {
        atlasCoords.push_back(atlas::PointLonLat(lonLatView(i, constants::eLongitude),
                                                 lonLatView(i, constants::eLatitude)));
      }
    } else {
      auto grid = atlas::functionspace::NodeColumns(field.functionspace()).mesh().grid();
      atlasCoords = getAtlasCoords(grid);
    }
  }
  return atlasCoords;
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getAtlasCoords(const atlas::Grid& grid) {
  oops::Log::debug() << "AtlasProcessor::getAtlasCoords()" << std::endl;
  std::vector<atlas::PointLonLat> atlasCoords;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    atlasCoords.resize(grid.size());
    std::copy(grid.lonlat().begin(), grid.lonlat().end(), atlasCoords.begin());
  }
  return atlasCoords;
}

std::vector<size_t> monio::AtlasProcessor::createLfricAtlasMap(
                                            const std::vector<atlas::PointLonLat>& atlasCoords,
                                            const std::vector<atlas::PointLonLat>& lfricCoords) {
  oops::Log::debug() << "AtlasProcessor::createLfricAtlasMap()" << std::endl;
  std::vector<size_t> lfricAtlasMap;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    // Essential check to ensure grid is configured to accommodate the data
    if (atlasCoords.size() != lfricCoords.size()) {
      throw std::runtime_error("AtlasProcessor::createLfricAtlasMap()> "
        "Configured grid is not compatible with input file...");
    }
    lfricAtlasMap.reserve(atlasCoords.size());

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
      lfricAtlasMap.push_back(idx);
    }
  }
  return lfricAtlasMap;
}

atlas::FieldSet monio::AtlasProcessor::getGlobalFieldSet(const atlas::FieldSet& fieldSet) {
  oops::Log::debug() << "AtlasProcessor::getGlobalFieldSet()" << std::endl;
  if (fieldSet.size() != 0) {
    atlas::FieldSet globalFieldSet;
    for (const auto& field : fieldSet) {
      if (field.metadata().get<bool>("global") == false) {
        atlas::util::Config atlasOptions = atlas::option::name(field.name()) |
                                           atlas::option::levels(field.levels()) |
                                           atlas::option::global(0);

        atlas::array::DataType atlasType = field.datatype();
        const auto& functionSpace = field.functionspace();
        atlas::Field globalField;
        switch (atlasType.kind()) {
          case atlasType.KIND_REAL64:
            globalField = functionSpace.createField<double>(atlasOptions);
            break;
          case atlasType.KIND_REAL32:
            globalField = functionSpace.createField<float>(atlasOptions);
            break;
          case atlasType.KIND_INT32:
            globalField = functionSpace.createField<int>(atlasOptions);
            break;
          default:
            throw std::runtime_error("AtlasProcessor::getGlobalFieldSet())> "
                                     "Data type not coded for...");
        }
        field.haloExchange();
        functionSpace.gather(field, globalField);
        globalFieldSet.add(globalField);
      } else {
        globalFieldSet.add(field);
      }
    }
    return globalFieldSet;
  } else {
    throw std::runtime_error("AtlasProcessor::getGlobalFieldSet()> FieldSet has zero fields...");
  }
}

atlas::idx_t monio::AtlasProcessor::getSizeOwned(const atlas::Field& field) {
  oops::Log::debug() << "AtlasProcessor::getSizeOwned()" << std::endl;
  atlas::Field ghostField = field.functionspace().ghost();
  atlas::idx_t sizeOwned = 0;
  auto ghostView = atlas::array::make_view<int, 1>(ghostField);
  for (atlas::idx_t i = ghostField.size() - 1; i >= 0; --i) {
    if (ghostView(i) == 0) {
      sizeOwned = i + 1;
      break;
    }
  }
  return sizeOwned;
}
