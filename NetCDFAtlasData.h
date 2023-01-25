
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

#include "NetCDFData.h"
#include "NetCDFMetadata.h"

#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/util/Point.h"

namespace lfriclite {
/// \brief Converts data to and from LFRic format and Atlas
class NetCDFAtlasData {
 public:
  NetCDFAtlasData(const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap,
                  const std::map<std::string,
                        std::tuple<std::string, int, size_t>>& fieldToMetadataMap,
                  const std::string gridName,
                  const std::string partitionerType,
                  const std::string meshType);

  ~NetCDFAtlasData();

  NetCDFAtlasData()                                  = delete;  //!< Deleted default constructor
  NetCDFAtlasData(const NetCDFAtlasData&)            = delete;  //!< Deleted copy constructor
  NetCDFAtlasData(NetCDFAtlasData&&)                 = delete;  //!< Deleted move constructor

  NetCDFAtlasData& operator=(const NetCDFAtlasData&) = delete;  //!< Deleted copy assign
  NetCDFAtlasData& operator=(NetCDFAtlasData&&)      = delete;       //!< Deleted move assign

  void toAtlasFields(NetCDFData* data);  // Needs to be called on default rank only.
  void scatterAtlasFields();  // Needs to be called on all PEs.
  void gatherAtlasFields();  // Needs to be called on all PEs
  void fromAtlasFields(NetCDFData* data);   // Needs to be called on default rank only

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
                          const std::map<std::string, NetCDFDataContainerBase*>& coordDataMap);
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
