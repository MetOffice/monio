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
class AtlasWriter {
 public:
  AtlasWriter(const eckit::mpi::Comm& mpiCommunicator,
              const atlas::idx_t mpiRankOwner);

  AtlasWriter()                               = delete;  //!< Deleted default constructor
  AtlasWriter(AtlasWriter&&)                  = delete;  //!< Deleted move constructor
  AtlasWriter(const AtlasWriter&)             = delete;  //!< Deleted copy constructor
  AtlasWriter& operator=( AtlasWriter&&)      = delete;  //!< Deleted move assignment
  AtlasWriter& operator=(const AtlasWriter&)  = delete;  //!< Deleted copy assignment

  void writeFieldSetToFile(const atlas::FieldSet& fieldSet,
                           const std::string outputFilePath);

  void writeIncrementsToFile(atlas::FieldSet& fieldSet,
                       const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                             monio::FileData& fileData,
                       const std::string& outputFilePath,
                       const bool isLfricFormat);

  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field,
                           const consts::FieldMetadata* fieldMetadata = nullptr,
                                 bool reverseDims = false);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const size_t fieldSize);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

 private:
  void populateMetadataAndDataWithFieldSet(Metadata& metadata,
                                           Data& data,
                                     const atlas::FieldSet& fieldSet);

  void populateMetadataAndDataWithLfricFieldSet(Metadata& metadata,
                                               Data& data,
                                         const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                                               atlas::FieldSet& fieldSet,
                                         const std::vector<size_t>& lfricToAtlasMap);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<size_t>& lfricToAtlasMap,
                       const size_t totalFieldSize);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<int> dimensions);

  void reconcileMetadataWithData(Metadata& metdata, Data& data);

  std::vector<std::shared_ptr<DataContainerBase>> convertLatLonToContainers(
                                        const std::vector<atlas::PointLonLat>& atlasCoords,
                                        const std::vector<std::string>& coordNames);

  atlas::Field processField(atlas::Field& inputField,
                      const consts::FieldMetadata& fieldMetadata);

  template<typename T>
  atlas::Field copySurfaceLevel(const atlas::Field& inputField,
                                const atlas::FunctionSpace& functionSpace,
                                const atlas::util::Config& atlasOptions);

  int atlasTypeToMonioEnum(atlas::array::DataType atlasType);

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;
};
}  // namespace monio
