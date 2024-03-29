/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <string>
#include <vector>

#include "DataContainerBase.h"

namespace monio {
/// \brief Concrete class for double precision numerical data of a NetCDF file.
class DataContainerDouble : public DataContainerBase {
 public:
  explicit DataContainerDouble(const std::string& name);

  DataContainerDouble()                                      = delete;  //!< Deleted default constr
  DataContainerDouble(DataContainerDouble&&)                 = delete;  //!< Deleted move construct
  DataContainerDouble(const DataContainerDouble&)            = delete;  //!< Deleted copy construct
  DataContainerDouble& operator=(DataContainerDouble&&)      = delete;  //!< Deleted move assign
  DataContainerDouble& operator=(const DataContainerDouble&) = delete;  //!< Deleted copy assign

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;

  std::vector<double>& getData();
  const std::vector<double>& getData() const;

  const double& getDatum(const size_t index);

  void setData(const std::vector<double> dataVector);
  void setDatum(const size_t index, const double datum);
  void setDatum(const double datum);

  void setSize(const int size);
  void clear();

 private:
  std::vector<double> dataVector_;
};
}  // namespace monio
