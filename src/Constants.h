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

  struct IncrementMetadata {
    std::string atlasName;
    std::string lfricName;
    std::string lfricIncName;
    std::string units;
    bool doCopySurfaceLevel;
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
    eString,
    eNumberOfDataTypes
  };

  enum eDimensions {
    eHorizontal,
    eVertical
  };

  enum eAttributeNames {
    eStandardName,
    eLongName,
    eUnitsName,
    eMeshName,
    eLocationName,
    eOnlineOperationName,
    eIntervalOperationName,
    eIntervalWriteName,
    eCellMethodsName,
    eCoordinatesName,
    eNumberOfAttributeNames
  };

  enum eIncrementVariables {
    eEastwardWind,
    eNorthwardWind,
    eExnerPressure,
    eDryDensity,
    ePotentialTemperature,
    eSpecificHumidity,
    eCloudWaterMass,
    eCloudIceMass,
    eNumberOfIncrementVariables
  };

  const std::string kDataTypeNames[eNumberOfDataTypes] = {
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

  // Used with MetadataLookup class - the following are Atlas names of increment variables
  const std::string  kIncrementVariables[eNumberOfIncrementVariables] {
    "eastward_wind",
    "northward_wind",
    "exner_levels_minus_one",
    "dry_air_density_levels_minus_one",
    "potential_temperature",
    "specific_humidity",
    "mass_content_of_cloud_liquid_water_in_atmosphere_layer",
    "mass_content_of_cloud_ice_in_atmosphere_layer",
  };

  const std::string kTimeDimName = "time_counter";
  const std::string kTimeVarName = "time_instant";
  const std::string kTimeOriginName = "time_origin";

  const std::string kTileDimName = "surface_tiles";
  const std::string kTileVarName = "tile_fraction";

  const std::string kHorizontalName = "nMesh2d_face";
  const std::string kVerticalFullName = "full_levels";
  const std::string kVerticalHalfName = "half_levels";


  const std::string kLfricMeshTerm = "Mesh2d";
  const std::string kLfricLonVarName = "Mesh2d_face_y";
  const std::string kLfricLatVarName = "Mesh2d_face_x";

  const std::string kLongitudeVarName = "longitude";
  const std::string kLatitudeVarName = "latitude";

  const std::string kTabSpace = "    ";
  const std::string kLevelsSearchTerm = "levels";
  const std::string kNotFoundError = "NOT_FOUND";

  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// Increment file metadata attributes
  /// Attribute names

  const std::string  kIncrementAttributeNames[eNumberOfAttributeNames] {
    "standard_name",
    "long_name",
    "units",
    "mesh",
    "location",
    "online_operation",
    "interval_operation",
    "interval_write",
    "cell_methods",
    "coordinates"
  };

  /// Attribute constant values
  const std::string kMeshValue = "Mesh2d";
  const std::string kLocationValue = "face";
  const std::string kOnlineOperationValue = "instant";  // Not needed?
  const std::string kIntervalOperationValue = "3600 s";  // Not needed?
  const std::string kntervalWriteValue = "3600 s";  // Not needed?
  const std::string kCellMethodsValue = "time: point";  // Not needed?
  const std::string kCoordinatesValue = "Mesh2d_face_y Mesh2d_face_x";

  const std::string  kIncrementVariableValues[eNumberOfAttributeNames] {
    kNotFoundError,  // Use AttributeLookup
    kNotFoundError,  // Use AttributeLookup
    kNotFoundError,  // Use AttributeLookup
    kMeshValue,
    kLocationValue,
    kOnlineOperationValue,
    kIntervalOperationValue,
    kntervalWriteValue,
    kCellMethodsValue,
    kCoordinatesValue
  };
  //////////////////////////////////////////////////////////////////////////////////////////////////

  const atlas::idx_t kMPIRankOwner = 0;

  const int kVerticalFullSize = 71;
  const int kVerticalHalfSize = 70;

  const std::vector<std::string> kLfricCoordVarNames = {kLfricLonVarName, kLfricLatVarName};
  const std::vector<std::string> kCoordVarNames = {kLongitudeVarName, kLatitudeVarName};
}  // namespace constants
}  // namespace monio
