/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace monio {
namespace consts {
/// Structs ////////////////////////////////////////////////////////////////////////////////////////

/// \brief This struct is used for interfacing with the Monio singleton and its intended use-cases
///        within the MO/JEDI context. MO model interfaces should include this class and build a
///        vector of these structs along with the field sets associated with reading and writing.
struct FieldMetadata {
  std::string lfricReadName;
  std::string lfricWriteName;
  std::string jediName;
  std::string units;
  int numberOfLevels;
  bool noFirstLevel;
};

/// Enums //////////////////////////////////////////////////////////////////////////////////////////

/// \brief Paired with struct FieldMetadata, above.
enum eFieldMetadata {
  eLfricReadName,
  eLfricWriteName,
  eJediName,
  eUnits,
  eNumberOfLevels,
  eNoFirstLevel
};

/// \brief For indexing spatial coordinates and associated data structures, e.g. kLfricCoordVarNames
enum eAtlasLonLat {
  eLongitude,
  eLatitude
};

/// \brief For indexing spatial coordinates of fields generically without reference to the globe.
enum eDimensions {
  eHorizontal,
  eVertical
};

/// \brief For indicating the naming convention adopted by input files, where applicable.
enum eNamingConventions {
  eLfricNaming,
  eJediNaming,
  eNotDefined
};

/// \brief Used for populating output files with the correct metadata associated with variable data.
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

/// \brief Adopted from the NetCDF library. Used here to indicate data types in MONIO where a
///        dependency on the NetCDF library should be avoided. Also indexes kDataTypeNames.
enum eDataTypes {
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

/// String/Views ///////////////////////////////////////////////////////////////////////////////////

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

const std::string_view kTabSpace = "    ";
const std::string_view kNotFoundError = "NOT_FOUND";

const std::string_view kProducedByName = "produced_by";
const std::string_view kProducedByString = "MONIO: Met Office NetCDF I/O";
const std::string_view kNamingConventionName = "naming_convention";

/// Multi-dimensional String/Views /////////////////////////////////////////////////////////////////

/// \brief Used with eDataTypes, above, for writing metadata to file or console.
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

/// \brief Used to set the metadata attribute names in output files.
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


const std::string_view  kIncrementVariableValues[eNumberOfAttributeNames] {
  kNotFoundError,  // Use AttributeLookup
  kNotFoundError,  // Use AttributeLookup
  kNotFoundError,  // Use AttributeLookup
  "Mesh2d",
  "face",
  "instant",
  "3600 s",
  "3600 s",
  "time: point",
  "Mesh2d_face_y Mesh2d_face_x"
};

/// \brief Needs to be a vector for use with call to search in Metadata::getNamingConvention.
const std::vector<std::string> kNamingConventions({
  "LFRic",
  "JEDI"
});

const std::vector<std::string> kMissingVariableNames({
  "TO BE DERIVED",      // LFRic-Lite - variables without names in the LFRic context
  "TO BE IMPLEMENTED",  // LFRic-Lite - theoretical variables that aren't used
  "none"                // LFRic-JEDI
});

const std::vector<std::string> kLfricCoordVarNames = {std::string(kLfricLonVarName),
                                                      std::string(kLfricLatVarName)};
const std::vector<std::string> kCoordVarNames = {std::string(kLongitudeVarName),
                                                 std::string(kLatitudeVarName)};

/// Numerical Constants ////////////////////////////////////////////////////////////////////////////

const int kMPIRankOwner = 0;

const int kVerticalFullSize = 71;
const int kVerticalHalfSize = 70;

}  // namespace consts
}  // namespace monio
