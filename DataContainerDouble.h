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
/// \brief Concrete class for double precision numerical data of a NetCDF file
class DataContainerDouble : public DataContainerBase {
 public:
  explicit DataContainerDouble(const std::string& name);

  DataContainerDouble()                           = delete;  //!< Deleted default constructor
  DataContainerDouble(const DataContainerDouble&) = delete;  //!< Deleted copy constructor
  DataContainerDouble(DataContainerDouble&&)      = delete;  //!< Deleted move constructor

  DataContainerDouble& operator=(const DataContainerDouble&) = delete;  //!< Deleted copy assign
  DataContainerDouble& operator=(DataContainerDouble&&)      = delete;  //!< Deleted move assign

  const std::string& getName() const;

  std::vector<double>& getData();
  const double* getDataPointer();
  const double& getDatum(const std::size_t index);

  void resetData();

 private:
  std::vector<double> dataVector_;
};
}  // namespace lfriclite
