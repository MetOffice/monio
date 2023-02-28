/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

#include "DataContainerBase.h"

namespace monio {
/// \brief Concrete class for integer numerical data of a NetCDF file
class DataContainerInt : public DataContainerBase {
 public:
  explicit DataContainerInt(const std::string& name);

  DataContainerInt()                        = delete;  //!< Deleted default constructor
  DataContainerInt(DataContainerInt&&)      = delete;  //!< Deleted move constructor
  DataContainerInt& operator=(const DataContainerInt&) = delete;  //!< Deleted copy assignment

  const std::string& getName() const;

  std::vector<int>& getData();
  const std::vector<int>& getData() const;

  const int* getDataPointer();
  const int& getDatum(const std::size_t index);

  void setData(const std::vector<int> dataVector);
  void setDatum(const size_t index, const int datum);
  void setDatum(const int datum);

  void setSize(const size_t size);
  void clear();

 private:
  std::vector<int> dataVector_;
};
}  // namespace monio
