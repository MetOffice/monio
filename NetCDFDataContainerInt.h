/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

#include "NetCDFDataContainerBase.h"

namespace lfriclite {
/// \brief Concrete class for integer numerical data of a NetCDF file
class NetCDFDataContainerInt : public NetCDFDataContainerBase {
 public:
  explicit NetCDFDataContainerInt(const std::string& name);

  NetCDFDataContainerInt()                      = delete;  //!< Deleted default constructor
  NetCDFDataContainerInt(const NetCDFDataContainerInt&) = delete;  //!< Deleted copy constructor
  NetCDFDataContainerInt(NetCDFDataContainerInt&&)      = delete;  //!< Deleted move constructor

  NetCDFDataContainerInt& operator=(const NetCDFDataContainerInt&) = delete;  //!< Deleted copy ass
  NetCDFDataContainerInt& operator=(NetCDFDataContainerInt&&) = delete;    //!< Deleted move assign

  const std::string& getName() const;

  std::vector<int>& getData();
  const int* getDataPointer();
  const int& getDatum(const std::size_t index);

  void resetData();

 private:
  std::vector<int> dataVector_;
};
}  // namespace lfriclite
