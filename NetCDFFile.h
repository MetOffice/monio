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

#include "NetCDFMetadata.h"

#include "atlas/field.h"

namespace lfriclite {
/// \brief Uses Unidata's C++ NetCDF library and holds handle to NetCDF file for reading or writing
class NetCDFFile {
 public:
  NetCDFFile(const std::string& filePath, const netCDF::NcFile::FileMode fileMode);
  ~NetCDFFile();

  NetCDFFile()                  = delete;  //!< Deleted default constructor
  NetCDFFile(const NetCDFFile&) = delete;  //!< Deleted copy constructor
  NetCDFFile(NetCDFFile&&)      = delete;  //!< Deleted move constructor

  NetCDFFile& operator=(const NetCDFFile&) = delete;  //!< Deleted copy assign
  NetCDFFile& operator=(NetCDFFile&&) = delete;       //!< Deleted move assign

  void readMetadata(NetCDFMetadata& metadata);
  void readMetadata(NetCDFMetadata& metadata,
                    const std::vector<std::string>& varNames);

  template<typename T> void readData(const std::string& varName,
                                     const int varSize,
                                     std::vector<T>& dataVec);

  template<typename T> void readField(const std::string& fieldName,
                                      const int varSize,
                                      const std::vector<size_t>& startVec,
                                      const std::vector<size_t>& countVec,
                                      std::vector<T>& dataVec);

  void writeMetadata(NetCDFMetadata& metadata);
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

  void readDimensions(NetCDFMetadata& metadata);
  void readVariables(NetCDFMetadata& metadata);
  void readVariables(NetCDFMetadata& metadata,
                     const std::vector<std::string>& variableNames);
  void readVariable(NetCDFMetadata& metadata, netCDF::NcVar var);
  void readAttributes(NetCDFMetadata& metadata);

  void writeDimensions(NetCDFMetadata& metadata);
  void writeVariables(NetCDFMetadata& metadata);
  void writeAttributes(NetCDFMetadata& metadata);

  std::unique_ptr<netCDF::NcFile> dataFile_;

  std::string filePath_;
  netCDF::NcFile::FileMode fileMode_;
};
}  // namespace lfriclite
