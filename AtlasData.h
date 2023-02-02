
/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <memory>
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

#include "eckit/mpi/Comm.h"

namespace monio {
/// \brief Converts data to and from LFRic format and Atlas
class AtlasData {
 public:
  AtlasData(const eckit::mpi::Comm& mpiCommunicator,
            const atlas::idx_t& mpiRankOwner,
            const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& coordDataMap,
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

  void initialiseNewFieldSet(const std::string& fieldSetName);

  void toFieldSet(const std::string& fieldSetName, const Data& data);
  void fromFieldSet(const std::string& fieldSetName, Data& data);

  atlas::FieldSet& getGlobalFieldSet(const std::string& fieldSetName);
  atlas::FieldSet& getLocalFieldSet(const std::string& fieldSetName);

 private:
  void toAtlasFields(const std::string& fieldSetName, const Data& data);
  void scatterAtlasFields(const std::string& fieldSetName);

  void gatherAtlasFields(const std::string& fieldSetName);
  void fromAtlasFields(const std::string& fieldSetName, Data& data);

  template<typename T> void fieldToAtlas(const std::string& fieldSetName,
                                         const std::string& atlasFieldName,
                                         const int& numLevels,
                                         const std::vector<T>& dataVec);

  template<typename T> void atlasToField(const std::string& fieldSetName,
                                         const std::string& atlasFieldName,
                                         const int& numLevels,
                                         std::vector<T>& dataVec);

  std::vector<atlas::PointLonLat> processLfricCoordData(
        const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& coordDataMap);
  void addField(const std::string& fieldSetName, atlas::Field field);

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t& mpiRankOwner_;
  const std::map<std::string, std::tuple<std::string, int, size_t>> fieldToMetadataMap_;

  atlas::CubedSphereGrid grid_;
  atlas::Mesh mesh_;
  atlas::functionspace::CubedSphereNodeColumns functionSpace_;

  std::vector<atlas::PointLonLat> pointsLonLat_;
  std::vector<atlas::PointLonLat> lfricCoordData_;
  std::vector<size_t> lfricToAtlasMap_;

  std::map<std::string, atlas::FieldSet> localFieldSetMap_;
  std::map<std::string, atlas::FieldSet> globalFieldSetMap_;
};
}  // namespace monio
