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
#include "FileData.h"

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
              const int mpiRankOwner);

  AtlasWriter()                               = delete;  //!< Deleted default constructor
  AtlasWriter(AtlasWriter&&)                  = delete;  //!< Deleted move constructor
  AtlasWriter(const AtlasWriter&)             = delete;  //!< Deleted copy constructor
  AtlasWriter& operator=( AtlasWriter&&)      = delete;  //!< Deleted move assignment
  AtlasWriter& operator=(const AtlasWriter&)  = delete;  //!< Deleted copy assignment

  void populateFileDataWithField(FileData& fileData,
                           const atlas::Field& field);

  void populateFileDataWithField(FileData& fileData,
                                 atlas::Field& field,
                           const std::string& writeName,
                           const bool copyFirstLevel = false);

 private:
  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);


  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<size_t>& lfricToAtlasMap);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<int> dimensions);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  int dimCount_ = 0;  // Used for automatic creation of dimension names
};
}  // namespace monio
