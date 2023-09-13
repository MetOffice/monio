/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "atlas/mesh/Mesh.h"

namespace monio {
namespace consts {
struct FieldMetadata {
  std::string lfricReadName;
  std::string lfricWriteName;
  std::string jediName;
  std::string units;
  int numberOfLevels;
  bool copyFirstLevel;
};

enum eAtlasLonLat {
  eLongitude,
  eLatitude
};

enum eFileNamingConventions {
  eLfricNaming,
  eJediNaming
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

enum eVariableMetadata {
  eLfricReadName,
  eLfricWriteName,
  eJediName,
  eUnits,
  eNumberOfLevels,
  eCopyFirstLevel
};

const std::string_view kDataTypeNames[eNumberOfDataTypes] = {
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
const std::string_view kIncrementVariables[eNumberOfIncrementVariables] {
  "eastward_wind",
  "northward_wind",
  "exner_levels_minus_one",
  "dry_air_density_levels_minus_one",
  "potential_temperature",
  "specific_humidity",
  "mass_content_of_cloud_liquid_water_in_atmosphere_layer",
  "mass_content_of_cloud_ice_in_atmosphere_layer",
};

const std::string_view kTimeDimName = "time_counter";
const std::string_view kTimeVarName = "time_instant";
const std::string_view kTimeOriginName = "time_origin";

const std::string_view kTileDimName = "surface_tiles";
const std::string_view kTileVarName = "tile_fraction";

const std::string_view kHorizontalName = "nMesh2d_face";
const std::string_view kVerticalFullName = "full_levels";
const std::string_view kVerticalHalfName = "half_levels";


const std::string_view kLfricMeshTerm = "Mesh2d";
const std::string_view kLfricLonVarName = "Mesh2d_face_y";
const std::string_view kLfricLatVarName = "Mesh2d_face_x";

const std::string_view kLongitudeVarName = "longitude";
const std::string_view kLatitudeVarName = "latitude";

const std::string_view kNamingConvention = "naming_convention";
const std::string_view kNamingJediName = "JEDI";
const std::string_view kNamingLfricName = "LFRic";

const std::string_view kTabSpace = "    ";
const std::string_view kLevelsSearchTerm = "levels";
const std::string_view kNotFoundError = "NOT_FOUND";

const std::string_view kToBeDerived = "TO BE DERIVED";
const std::string_view kToBeImplemented = "TO BE IMPLEMENTED";

const std::string_view kProducedByName = "Produced_by";
const std::string_view kProducedByString = "MONIO: Met Office NetCDF I/O";

const std::string_view kDataFormatName = "Data_format";
const std::string_view kDataFormatAtlas = "Atlas";
const std::string_view kDataFormatLfric = "LFRic";

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Increment file metadata attributes
/// Attribute names

const std::string_view  kIncrementAttributeNames[eNumberOfAttributeNames] {
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
const std::string_view kMeshValue = "Mesh2d";
const std::string_view kLocationValue = "face";
const std::string_view kOnlineOperationValue = "instant";  // Not needed?
const std::string_view kIntervalOperationValue = "3600 s";  // Not needed?
const std::string_view kntervalWriteValue = "3600 s";  // Not needed?
const std::string_view kCellMethodsValue = "time: point";  // Not needed?
const std::string_view kCoordinatesValue = "Mesh2d_face_y Mesh2d_face_x";

const std::string_view  kIncrementVariableValues[eNumberOfAttributeNames] {
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

const int kMPIRankOwner = 0;

const int kVerticalFullSize = 71;
const int kVerticalHalfSize = 70;

const std::vector<std::string> kLfricCoordVarNames = {std::string(kLfricLonVarName),
                                                      std::string(kLfricLatVarName)};
const std::vector<std::string> kCoordVarNames = {std::string(kLongitudeVarName),
                                                 std::string(kLatitudeVarName)};
}  // namespace consts
}  // namespace monio
