/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#include "UtilsAtlas.h"

#include <algorithm>
#include <numeric>

#include "atlas/functionspace.h"
#include "atlas/grid/Iterator.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/util/KDTree.h"
#include "oops/util/Logger.h"

#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "Utils.h"

namespace monio {
namespace utilsatlas {
std::vector<atlas::PointLonLat> getLfricCoords(
                     const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData) {
  std::vector<atlas::PointLonLat> lfricCoords;
  if (coordData.size() == 2) {
    std::array<std::vector<float>, 2> coordVectorArray;
    int coordCount = 0;
    for (auto& coordContainer : coordData) {
      // LFRic coordinate data are currently stored as floats
      if (coordContainer->getType() == consts::eFloat) {
        std::shared_ptr<DataContainerFloat> cooordContainerFloat =
                  std::static_pointer_cast<DataContainerFloat>(coordContainer);
        std::vector<float> coordData = cooordContainerFloat->getData();
        coordVectorArray[coordCount] = coordData;
        coordCount++;
      } else {
        utils::throwException("utilsatlas::getLfricCoords()> "
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
      utils::throwException("utilsatlas::getLfricCoords()> "
          "Incorrect number of coordinate axes...");
  }
  return lfricCoords;
}

std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Field& field) {
  std::vector<atlas::PointLonLat> atlasCoords;
  if (field.metadata().get<bool>("global") == false) {
    auto lonLatField = field.functionspace().lonlat();
    auto lonLatView = atlas::array::make_view<double, 2>(lonLatField);
    for (int i = 0; i < getHorizontalSize(field); ++i) {
      atlasCoords.push_back(atlas::PointLonLat(lonLatView(i, consts::eLongitude),
                                               lonLatView(i, consts::eLatitude)));
    }
  } else {
    auto grid = atlas::functionspace::NodeColumns(field.functionspace()).mesh().grid();
    atlasCoords = getAtlasCoords(grid);
  }
  return atlasCoords;
}

std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Grid& grid) {
  std::vector<atlas::PointLonLat> atlasCoords;
  atlasCoords.resize(grid.size());
  std::copy(grid.lonlat().begin(), grid.lonlat().end(), atlasCoords.begin());
  return atlasCoords;
}

std::vector<std::shared_ptr<monio::DataContainerBase>> convertLatLonToContainers(
                        const std::vector<atlas::PointLonLat>& atlasCoords,
                        const std::vector<std::string>& coordNames) {
  std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
  std::shared_ptr<DataContainerDouble> lonContainer =
            std::make_shared<DataContainerDouble>(coordNames[consts::eLongitude]);
  std::shared_ptr<DataContainerDouble> latContainer =
            std::make_shared<DataContainerDouble>(coordNames[consts::eLatitude]);
  for (const auto& atlasCoord : atlasCoords) {
    lonContainer->setDatum(atlasCoord.lon());
    latContainer->setDatum(atlasCoord.lat());
  }
  coordContainers.push_back(lonContainer);
  coordContainers.push_back(latContainer);
  return coordContainers;
}

std::vector<size_t> createLfricAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                        const std::vector<atlas::PointLonLat>& lfricCoords) {
  std::vector<size_t> lfricAtlasMap;
  // Essential check to ensure grid is configured to accommodate the data
  if (atlasCoords.size() != lfricCoords.size()) {
    utils::throwException("utilsatlas::createLfricAtlasMap()> "
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
  return lfricAtlasMap;
}

atlas::Field getGlobalField(const atlas::Field& field) {
  if (field.metadata().get<bool>("global") == false) {
    atlas::array::DataType atlasType = field.datatype();
    atlas::util::Config atlasOptions = atlas::option::name(field.name()) |
                                       atlas::option::levels(field.levels()) |
                                       atlas::option::datatype(atlasType) |
                                       atlas::option::global(0);
    if (atlasType != atlasType.KIND_REAL64 &&
        atlasType != atlasType.KIND_REAL32 &&
        atlasType != atlasType.KIND_INT32) {
        utils::throwException("utilsatlas::getGlobalFieldSet())> Data type not coded for...");
    }
    const auto& functionSpace = field.functionspace();
    atlas::Field globalField = functionSpace.createField(atlasOptions);
    field.haloExchange();
    functionSpace.gather(field, globalField);
    return globalField;
  } else {
    return field;
  }
}

atlas::FieldSet getGlobalFieldSet(const atlas::FieldSet& fieldSet) {
  if (fieldSet.size() != 0) {
    atlas::FieldSet globalFieldSet;
    for (const auto& field : fieldSet) {
      globalFieldSet.add(getGlobalField(field));
    }
    return globalFieldSet;
  } else {
    utils::throwException("utilsatlas::getGlobalFieldSet()> FieldSet has zero fields...");
  }
}

int getHorizontalSize(const atlas::Field& field) {
  atlas::Field ghostField = field.functionspace().ghost();
  int size = 0;
  auto ghostView = atlas::array::make_view<int, 1>(ghostField);
  for (int i = ghostField.size() - 1; i >= 0; --i) {
    if (ghostView(i) == 0) {
      size = i + 1;
      break;
    }
  }
  return size;
}

int getGlobalDataSize(const atlas::Field& field) {
  std::vector<int> dimVec = field.shape();
  int size = 1;
  for (const auto& dim : dimVec) {
    size *= dim;
  }
  return size;
}

int atlasTypeToMonioEnum(atlas::array::DataType atlasType) {
  switch (atlasType.kind()) {
  case atlasType.KIND_INT32: {
    return consts::eInt;
  }
  case atlasType.KIND_REAL32: {
    return consts::eFloat;
  }
  case atlasType.KIND_REAL64: {
    return consts::eDouble;
  }
  default:
    utils::throwException("utilsatlas::atlasTypeToMonioEnum()> Data type not coded for...");
  }
}

bool compareFieldSets(const atlas::FieldSet& aSet, const atlas::FieldSet& bSet) {
  for (auto& a : aSet) {
    if (compareFields(a, bSet[a.name()]) == false) {
      return false;
    }
  }
  return true;
}

bool compareFields(const atlas::Field& a, const atlas::Field& b) {
  const auto aView = atlas::array::make_view<const double, 2>(a);
  const auto bView = atlas::array::make_view<const double, 2>(b);
  std::vector<int> dimVec = a.shape();
  for (int j = 0; j < dimVec[consts::eVertical]; ++j) {
    for (int i = 0; i < dimVec[consts::eHorizontal]; ++i) {
      if (aView(i, j) != bView(i, j)) {
        return false;
      }
    }
  }
  return true;
}
}  // namespace utilsatlas
}  // namespace monio
