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
/// \brief Concrete class for single precision numerical data of a NetCDF file
class DataContainerFloat : public DataContainerBase {
 public:
  explicit DataContainerFloat(const std::string& name);

  DataContainerFloat()                          = delete;  //!< Deleted default constructor
  DataContainerFloat(const DataContainerFloat&) = delete;  //!< Deleted copy constructor
  DataContainerFloat(DataContainerFloat&&)      = delete;  //!< Deleted move constructor

  DataContainerFloat& operator=(const DataContainerFloat&) = delete;  //!< Deleted copy assignment
  DataContainerFloat& operator=(DataContainerFloat&&)      = delete;  //!< Deleted move assignment

  const std::string& getName() const;

  std::vector<float>& getData();
  const std::vector<float>& getData() const;

  const float* getDataPointer();
  const float& getDatum(const std::size_t index);

  void resetData();

 private:
  std::vector<float> dataVector_;
};
}  // namespace lfriclite
