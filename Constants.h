/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

#include "atlas/mesh/Mesh.h"

namespace monio {
namespace constants {

  enum eFieldMetadata {
      eAtlasFieldName,
      eDataType,
      eNumLevels
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

  const atlas::idx_t kMPIRankOwner = 0;

  const std::string kTabSpace = "    ";
  const std::string kLevelsSearchTerm = "levels";
}  // namespace constants
}  // namespace monio
