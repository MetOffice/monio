/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <string>
#include <vector>

#include "DataContainerBase.h"

namespace monio {
/// \brief Concrete class for integer numerical data of a NetCDF file.
class DataContainerInt : public DataContainerBase {
 public:
  explicit DataContainerInt(const std::string& name);

  DataContainerInt()                                   = delete;  //!< Deleted default constructor
  DataContainerInt(DataContainerInt&&)                 = delete;  //!< Deleted move constructor
  DataContainerInt(const DataContainerInt&)            = delete;  //!< Deleted copy constructor
  DataContainerInt& operator=(DataContainerInt&&)      = delete;  //!< Deleted move assignment
  DataContainerInt& operator=(const DataContainerInt&) = delete;  //!< Deleted copy assignment

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;

  std::vector<int>& getData();
  const std::vector<int>& getData() const;

  const int& getDatum(const size_t index);

  void setData(const std::vector<int> dataVector);
  void setDatum(const size_t index, const int datum);
  void setDatum(const int datum);

  void setSize(const int size);
  void clear();

 private:
  std::vector<int> dataVector_;
};
}  // namespace monio
