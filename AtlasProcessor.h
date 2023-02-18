/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <vector>

#include "Constants.h"
#include "Data.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Metadata.h"

#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/util/Point.h"
#include "eckit/mpi/Comm.h"

namespace monio {
class AtlasProcessor {
 public:
  AtlasProcessor(const eckit::mpi::Comm& mpiCommunicator,
                 const atlas::idx_t& mpiRankOwner);

  AtlasProcessor()                                 = delete;  //!< Deleted default constructor
  AtlasProcessor(const AtlasProcessor&)            = delete;  //!< Deleted copy constructor
  AtlasProcessor& operator=(const AtlasProcessor&) = delete;  //!< Deleted copy assignment

  void initAtlasLonLat(atlas::CubedSphereGrid& grid);

  // Used where container type is not known - vector cannot be accessed from base class
  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer);

  // Used where container type is known
  template<typename T> void populateField(atlas::Field& field,
                                          const std::vector<T>& dataVec);

  void populateDataWithField(Data& data, const atlas::Field field);

  void populateDataWithFieldSet(Data& data, const atlas::FieldSet& fieldSet);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                            const atlas::Field& field);
  void processLfricCoordData(
      const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& coordDataMap,
      const std::vector<atlas::PointLonLat>& atlasCoords);

  void setLfricToAtlasMap(const std::vector<size_t>& lfricToAtlasMap);

 private:
  void createLfricToAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                             const std::vector<atlas::PointLonLat>& lfricCoords);

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t& mpiRankOwner_;

  std::vector<atlas::PointLonLat> atlasCoords_;
  std::vector<atlas::PointLonLat> lfricCoords_;

  std::vector<size_t> lfricToAtlasMap_;
};
}  // namespace monio
