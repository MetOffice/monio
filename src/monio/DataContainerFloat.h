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
/// \brief Concrete class for single precision numerical data of a NetCDF file.
class DataContainerFloat : public DataContainerBase {
 public:
  explicit DataContainerFloat(const std::string& name);

  DataContainerFloat()                                     = delete;  //!< Deleted default construc
  DataContainerFloat(DataContainerFloat&&)                 = delete;  //!< Deleted move constructor
  DataContainerFloat(const DataContainerFloat&)            = delete;  //!< Deleted copy constructor
  DataContainerFloat& operator=(DataContainerFloat&&)      = delete;  //!< Deleted move assignment
  DataContainerFloat& operator=(const DataContainerFloat&) = delete;  //!< Deleted copy assignment

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;

  std::vector<float>& getData();
  const std::vector<float>& getData() const;

  const float& getDatum(const size_t index);

  void setData(const std::vector<float> dataVector);
  void setDatum(const size_t index, const float datum);
  void setDatum(const float datum);

  void setSize(const int size);
  void clear();

 private:
  std::vector<float> dataVector_;
};
}  // namespace monio
