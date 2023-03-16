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

  AtlasProcessor()                                 = delete;  //!< Deleted default constructor
  AtlasProcessor(const AtlasProcessor&)            = delete;  //!< Deleted copy constructor
  AtlasProcessor& operator=(const AtlasProcessor&) = delete;  //!< Deleted copy assignment

  static void writeFieldSetToFile(atlas::FieldSet fieldSet, std::string outputFilePath);
  void writeIncrementsToFile(atlas::FieldSet fieldSet, std::string outputFilePath);

template<typename T>
std::vector<T>& getDataVecFromContainer(std::shared_ptr<monio::DataContainerBase>& dataContainer);

  void createGlobalFieldSet(const atlas::FieldSet& localFieldSet,
                                  atlas::FieldSet& globalFieldSet);

  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Field field);
  std::vector<atlas::PointLonLat> getAtlasCoords(const atlas::Grid& grid);
  std::vector<atlas::PointLonLat> getLfricCoords(
                    const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData);
  std::vector<size_t> createLfricAtlasMap(const std::vector<atlas::PointLonLat>& atlasCoords,
                                          const std::vector<atlas::PointLonLat>& lfricCoords);

  std::vector<std::shared_ptr<DataContainerBase>> convertLatLonToContainers(
                                        const std::vector<atlas::PointLonLat>& atlasCoords,
                                        const std::vector<std::string>& coordNames);

  // Used where container type is not known - vector cannot be accessed from base class
  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const std::vector<size_t>& lfricToAtlasMap);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const size_t fieldSize);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);

  // Used where container type is known
  template<typename T> void populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec,
                                    const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

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

  int getSizeOwned(const atlas::Field field);

 private:
  int atlasTypeToMonioEnum(atlas::array::DataType atlasType);

  const eckit::mpi::Comm& mpiCommunicator_;
  const atlas::idx_t mpiRankOwner_;
};
}  // namespace monio
