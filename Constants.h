/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

#include "atlas/mesh/Mesh.h"

namespace monio {
namespace constants {

  struct FieldMetadata {
    std::string lfricName;
    std::string atlasName;
    size_t numLevels;
    size_t fieldSize;
    int dataType;
  };

  enum eAtlasLonLat {
    eLongitude,
    eLatitude
  };

  enum eDataTypes {  // Used in Reader and File
    eByte,
    eChar,
    eShort,
    eInt,
    eFloat,
    eDouble,
    eUByte,
    eUShort,
    eUInt,
    eUInt64,
    eString
  };

  enum eDimensions {
    eHorizontal,
    eVertical
  };

  const std::string kDataTypeNames[eString + 1] = {
    "byte",
    "char",
    "short",
    "int",
    "float",
    "double",
    "unsigned byte",
    "unsigned short",
    "unsigned int",
    "long int",
    "std::string"
  };

  const std::string kTimeDimName = "time_counter";
  const std::string kTimeVarName = "time_instant";
  const std::string kTimeOriginName = "time_origin";

  const std::string kTileDimName = "surface_tiles";
  const std::string kTileVarName = "tile_fraction";

  const std::string kHorizontalName = "nMesh2d_face";
  const std::string kVerticalFullName = "full_levels";
  const std::string kVerticalHalfName = "half_levels";

  const std::string kLfricLonVarName = "Mesh2d_face_y";
  const std::string kLfricLatVarName = "Mesh2d_face_x";

  const std::string kLongitudeVarName = "longitude";
  const std::string kLatitudeVarName = "latitude";

  const std::string kTabSpace = "    ";
  const std::string kLevelsSearchTerm = "levels";
  const std::string kNotFoundError = "NOT_FOUND";

  const atlas::idx_t kMPIRankOwner = 0;

  const int kVerticalFullSize = 71;
  const int kVerticalHalfSize = 70;

  const std::vector<std::string> kLfricCoordVarNames = {kLfricLonVarName, kLfricLatVarName};
  const std::vector<std::string> kCoordVarNames = {kLongitudeVarName, kLatitudeVarName};
}  // namespace constants
}  // namespace monio
