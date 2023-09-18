/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
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
  atlas::Field getWriteField(atlas::Field& inputField,
                           const std::string& writeName,
                           const bool copyFirstLevel);

  template<typename T>
  atlas::Field copySurfaceLevel(const atlas::Field& inputField,
                                const atlas::FunctionSpace& functionSpace,
                                const atlas::util::Config& atlasOptions);

  int getHorizontalSize(const atlas::Field& field);  // Just 2D size. Any field.
  int getGlobalDataSize(const atlas::Field& field);  // Full 3D size of data. Global fields only

  int atlasTypeToMonioEnum(atlas::array::DataType atlasType);
}  // namespace utilsatlas
}  // namespace monio
