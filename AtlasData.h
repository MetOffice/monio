
/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "Data.h"
#include "Metadata.h"

#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/util/Point.h"

namespace monio {
/// \brief Converts data to and from LFRic format and Atlas
class AtlasData {
 public:
  AtlasData(const std::map<std::string, DataContainerBase*>& coordDataMap,
            const std::map<std::string, std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
            const std::string gridName,
            const std::string partitionerType,
            const std::string meshType);

  ~AtlasData();

  AtlasData()                            = delete;  //!< Deleted default constructor
  AtlasData(const AtlasData&)            = delete;  //!< Deleted copy constructor
  AtlasData(AtlasData&&)                 = delete;  //!< Deleted move constructor

  AtlasData& operator=(const AtlasData&) = delete;  //!< Deleted copy assignment
  AtlasData& operator=(AtlasData&&)      = delete;  //!< Deleted move assignment

  void toAtlasFields(Data* data);  // Needs to be called on default rank only.
  void scatterAtlasFields();  // Needs to be called on all PEs.
  void gatherAtlasFields();  // Needs to be called on all PEs
  void fromAtlasFields(Data* data);   // Needs to be called on default rank only

  atlas::FieldSet getGlobalFieldSet();
  atlas::FieldSet getFieldSet();

 private:
  template<typename T> void fieldToAtlas(const std::string& atlasFieldName,
                                         const int& numLevels,
                                         const std::vector<T>& dataVec);
  template<typename T> void atlasToField(const std::string& atlasFieldName,
                                         const int& numLevels,
                                         std::vector<T>& dataVec);
  std::vector<atlas::PointLonLat> processLfricCoordData(
                          const std::map<std::string, DataContainerBase*>& coordDataMap);
  void addField(atlas::Field field);

  std::map<std::string, std::tuple<std::string, int, size_t>> fieldToMetadataMap_;

  atlas::CubedSphereGrid grid_;
  atlas::Mesh mesh_;
  atlas::functionspace::CubedSphereNodeColumns functionSpace_;

  std::vector<atlas::PointLonLat> pointsLonLat_;
  std::vector<atlas::PointLonLat> lfricCoordData_;
  std::vector<size_t> lfricToAtlasMap_;

  atlas::FieldSet fieldSet_;
  atlas::FieldSet globalFieldSet_;
};
}  // namespace lfriclite
