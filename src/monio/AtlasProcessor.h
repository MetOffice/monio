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

#include "Constants.h"
#include "Data.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "FileData.h"
#include "Metadata.h"

#include "atlas/array/DataType.h"
#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/util/Point.h"
#include "eckit/mpi/Comm.h"

namespace monio {
class AtlasProcessor {
 public:
  AtlasProcessor(const eckit::mpi::Comm& mpiCommunicator,
                 const atlas::idx_t mpiRankOwner);

  AtlasProcessor()                                  = delete;  //!< Deleted default constructor
  AtlasProcessor(AtlasProcessor&&)                  = delete;  //!< Deleted move constructor
  AtlasProcessor(const AtlasProcessor&)             = delete;  //!< Deleted copy constructor
  AtlasProcessor& operator=( AtlasProcessor&&)      = delete;  //!< Deleted move assignment
  AtlasProcessor& operator=(const AtlasProcessor&)  = delete;  //!< Deleted copy assignment

  std::vector<atlas::PointLonLat> getLfricCoords(
                    const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData);

  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Field& field);
  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Grid& grid);

  std::vector<size_t> createLfricAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                          const std::vector<atlas::PointLonLat>& lfricCoords);

  atlas::FieldSet getGlobalFieldSet(const atlas::FieldSet& fieldSet);

  atlas::idx_t getSizeOwned(const atlas::Field& field);

 private:
  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;
};
}  // namespace monio
