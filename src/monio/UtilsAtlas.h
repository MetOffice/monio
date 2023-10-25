/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "atlas/array/DataType.h"
#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/util/Point.h"
#include "eckit/mpi/Comm.h"

#include "Constants.h"
#include "DataContainerBase.h"

namespace monio {
/// \brief Contains helper functions specifically for processing Atlas data.
namespace utilsatlas {
  std::vector<atlas::PointLonLat> getLfricCoords(
                    const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData);

  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Field& field);
  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Grid& grid);

  std::vector<std::shared_ptr<DataContainerBase>> convertLatLonToContainers(
                                        const std::vector<atlas::PointLonLat>& atlasCoords,
                                        const std::vector<std::string>& coordNames);

  std::vector<size_t> createLfricAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                          const std::vector<atlas::PointLonLat>& lfricCoords);

  atlas::FieldSet getGlobalFieldSet(const atlas::FieldSet& fieldSet);

  atlas::Field getGlobalField(const atlas::Field& field);

  int getHorizontalSize(const atlas::Field& field);  // Just 2D size. Any field.
  int getGlobalDataSize(const atlas::Field& field);  // Full 3D size of data. Global fields only

  int atlasTypeToMonioEnum(atlas::array::DataType atlasType);

  bool compareFieldSets(const atlas::FieldSet& aSet, const atlas::FieldSet& bSet);
  bool compareFields(const atlas::Field& a, const atlas::Field& b);
}  // namespace utilsatlas
}  // namespace monio
