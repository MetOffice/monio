/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <netcdf>

#include <memory>
#include <string>
#include <vector>

#include "Metadata.h"

#include "atlas/field.h"

namespace monio {
/// \brief Uses Unidata's C++ NetCDF library and holds handle to NetCDF file for reading or writing
class File {
 public:
  File(const std::string& filePath, const netCDF::NcFile::FileMode fileMode);
  ~File();

  File()                       = delete;  //!< Deleted default constructor
  File(const File&)            = delete;  //!< Deleted copy constructor
  File(File&&)                 = delete;  //!< Deleted move constructor

  File& operator=(const File&) = delete;  //!< Deleted copy assignment
  File& operator=(File&&)      = delete;  //!< Deleted move assignment

  void readMetadata(Metadata& metadata);
  void readMetadata(Metadata& metadata,
                    const std::vector<std::string>& varNames);

  template<typename T> void readData(const std::string& varName,
                                     const int varSize,
                                     std::vector<T>& dataVec);

  template<typename T> void readField(const std::string& fieldName,
                                      const int varSize,
                                      const std::vector<size_t>& startVec,
                                      const std::vector<size_t>& countVec,
                                      std::vector<T>& dataVec);

  void writeMetadata(Metadata& metadata);
  template<typename T> void writeData(const std::string& varName,
                                      const std::vector<T>& dataVec);

  const std::string& getPath();
  const netCDF::NcFile::FileMode& getFileMode();

  const bool isRead();
  const bool isWrite();

  void setPath(const std::string filePath);
  void setFileMode(const netCDF::NcFile::FileMode fileMode);

 private:
  netCDF::NcFile* getFile();

  void readDimensions(Metadata& metadata);
  void readVariables(Metadata& metadata);
  void readVariables(Metadata& metadata,
                     const std::vector<std::string>& variableNames);
  void readVariable(Metadata& metadata, netCDF::NcVar var);
  void readAttributes(Metadata& metadata);

  void writeDimensions(Metadata& metadata);
  void writeVariables(Metadata& metadata);
  void writeAttributes(Metadata& metadata);

  std::unique_ptr<netCDF::NcFile> dataFile_;

  std::string filePath_;
  netCDF::NcFile::FileMode fileMode_;
};
}  // namespace monio
