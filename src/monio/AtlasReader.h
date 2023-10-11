/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
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
/// \brief Used during file reading. Encapsulates the dependency upon Atlas. Includes functions to
///        populate Atlas fields with data read from files.
class AtlasReader {
 public:
  AtlasReader(const eckit::mpi::Comm& mpiCommunicator,
              const int mpiRankOwner);

  AtlasReader()                               = delete;  //!< Deleted default constructor
  AtlasReader(AtlasReader&&)                  = delete;  //!< Deleted move constructor
  AtlasReader(const AtlasReader&)             = delete;  //!< Deleted copy constructor
  AtlasReader& operator=( AtlasReader&&)      = delete;  //!< Deleted move assignment
  AtlasReader& operator=(const AtlasReader&)  = delete;  //!< Deleted copy assignment

  /// \brief  Provides the entry point to the class by calling relevant, private functions.
  ///         Including a call to get a field formatted for reading.
  void populateFieldWithFileData(atlas::Field& field,
                           const FileData& fileData,
                           const consts::FieldMetadata& fieldMetadata,
                           const std::string& readName);

 private:
  /// \brief Called from the entry point. Derives container type, makes the call to populate a field
  ///        with data.
  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const bool copyFirstLevel = false);

  /// \brief Not currently used, but could be. Derives container type, makes the call to populate a
  ///        field with data where data order isn't relevant.
  void populateFieldWithDataContainer(atlas::Field& field,
                                const std::shared_ptr<monio::DataContainerBase>& dataContainer);

  /// \brief Provides function to populate a field with read data and skips data on zeroth level
  ///        where applicable.
  template<typename T> void populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec,
                                    const std::vector<size_t>& lfricToAtlasMap,
                                    const bool copyFirstLevel);

  /// \brief Not currently used, but used to populate a field where data order isn't relevant.
  template<typename T> void populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec);

  /// \brief Returns a formatted field without a zeroth level, where applicable.
  atlas::Field getReadField(atlas::Field& inputField, const bool noFirstLevel);

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;
};
}  // namespace monio
