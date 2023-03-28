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

  static void writeFieldSetToFile(const atlas::FieldSet fieldSet,
                                  const std::string outputFilePath);

  void writeIncrementsToFile(const atlas::FieldSet fieldSet,
                             const std::vector<std::string>& varNames,
                             monio::FileData& fileData,
                             const std::string outputFilePath);

  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Field field);
  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Grid& grid);
  std::vector<atlas::PointLonLat> getLfricCoords(
                    const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData);
  std::vector<size_t> createLfricAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                          const std::vector<atlas::PointLonLat>& lfricCoords);


  // Used where container type is not known - vector cannot be accessed from base class
  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const std::vector<size_t>& lfricToAtlasMap);

  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const size_t fieldSize);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);

  void populateFieldSetWithData(atlas::FieldSet& fieldSet, const Data& data);
  void populateFieldSetWithData(atlas::FieldSet& fieldSet,
                          const Data& data,
                          const std::vector<std::string>& fieldNames);

 private:
  std::vector<std::shared_ptr<DataContainerBase>> convertLatLonToContainers(
                                        const std::vector<atlas::PointLonLat>& atlasCoords,
                                        const std::vector<std::string>& coordNames);

  void populateMetadataAndDataWithLfricFieldSet(Metadata& metadata,
                                                Data& data,
                                          const atlas::FieldSet& fieldSet,
                                          const std::vector<size_t>& lfricToAtlasMap);

  void populateMetadataAndDataWithFieldSet(Metadata& metadata,
                                           Data& data,
                                     const atlas::FieldSet& fieldSet);

  void populateMetadataWithField(Metadata& metadata, const atlas::Field field);

  void populateDataWithField(Data& data,
                       const atlas::Field field,
                       const std::vector<size_t>& lfricToAtlasMap,
                       const size_t totalFieldSize);

  void populateDataWithField(Data& data,
                       const atlas::Field field,
                       const std::vector<int> dimensions);

  template<typename T> void populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec,
                                    const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

  void reconcileMetadataWithData(Metadata& metdata, Data& data);
  int atlasTypeToMonioEnum(atlas::array::DataType atlasType);
  int getSizeOwned(const atlas::Field field);

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;
};
}  // namespace monio
