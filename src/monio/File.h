/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <netcdf>

#include <memory>
#include <string>
#include <vector>

#include "Metadata.h"

namespace monio {
/// \brief Uses Unidata's C++ NetCDF library and holds handle to NetCDF file for reading or writing.
class File {
 public:
  File(const std::string& filePath, const netCDF::NcFile::FileMode fileMode);
  ~File();

  File()                       = delete;  //!< Deleted default constructor
  File(File&&)                 = delete;  //!< Deleted move constructor
  File(const File&)            = delete;  //!< Deleted copy constructor
  File& operator=(File&&)      = delete;  //!< Deleted move assignment
  File& operator=(const File&) = delete;  //!< Deleted copy assignment

  void close();
  /// \brief Read all metadata.
  void readMetadata(Metadata& metadata);
  /// \brief Read dimensions, attributes, and a subset of variables metadata.
  void readMetadata(Metadata& metadata,
              const std::vector<std::string>& varNames);

  /// \brief Read a complete variable.
  template<typename T> void readSingleDatum(const std::string& varName,
                                            std::vector<T>& dataVec);
  /// \brief Read a subset of a variable. Usually at different positions in a time series.
  template<typename T> void readFieldDatum(const std::string& fieldName,
                                           const std::vector<size_t>& startVec,
                                           const std::vector<size_t>& countVec,
                                           std::vector<T>& dataVec);

  void writeMetadata(const Metadata& metadata);

  template<typename T> void writeSingleDatum(const std::string& varName,
                                             const std::vector<T>& dataVec);

 private:
  netCDF::NcFile& getFile();

  void readDimensions(Metadata& metadata);
  void readVariables(Metadata& metadata);
  void readVariables(Metadata& metadata,
                     const std::vector<std::string>& variableNames);
  void readVariable(Metadata& metadata, netCDF::NcVar var);
  void readAttributes(Metadata& metadata);

  void writeDimensions(const Metadata& metadata);
  void writeVariables(const Metadata& metadata);
  void writeAttributes(const Metadata& metadata);

  std::unique_ptr<netCDF::NcFile> dataFile_;

  std::string filePath_;
  netCDF::NcFile::FileMode fileMode_;
};
}  // namespace monio
