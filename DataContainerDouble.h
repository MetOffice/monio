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
  DataContainerDouble& operator=(const DataContainerDouble&) = delete;  //!< Deleted copy assign

  const std::string& getName() const;

  std::vector<double>& getData();
  const std::vector<double>& getData() const;

  const double* getDataPointer();
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
