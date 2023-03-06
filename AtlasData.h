
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

#include "AtlasProcessor.h"
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
            const atlas::idx_t mpiRankOwner,
            const std::vector<monio::constants::FieldMetadata> fieldToMetadataVec,
            const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData,
            const std::string gridName,
            const std::string partitionerType,
            const std::string meshType);

  AtlasData()                            = delete;  //!< Deleted default constructor
  AtlasData(const AtlasData&)            = delete;  //!< Deleted copy constructor
  AtlasData& operator=(const AtlasData&) = delete;  //!< Deleted copy assignment

  void initialiseMemberFieldSet();

  void toFieldSet(const Data& data);
  void fromFieldSet(Data& data);

  atlas::FieldSet& getGlobalFieldSet();
  atlas::FieldSet& getLocalFieldSet();

 private:
  void scatterAtlasFields();
  void gatherAtlasFields();

  void addField(atlas::Field& field);

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;
  const std::vector<monio::constants::FieldMetadata> fieldToMetadataVec_;

  monio::AtlasProcessor atlasProcessor_;

  atlas::CubedSphereGrid grid_;
  atlas::Mesh mesh_;
  atlas::functionspace::CubedSphereNodeColumns functionSpace_;

  atlas::FieldSet localFieldSet_;
  atlas::FieldSet globalFieldSet_;

  std::vector<atlas::PointLonLat> lfricCoords_;
  std::vector<atlas::PointLonLat> atlasCoords_;
  std::vector<size_t> lfricAtlasMap_;
};
}  // namespace monio
