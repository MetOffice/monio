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

namespace monio {
/// \brief Pure abstract base class for data read from or to be written to a NetCDF file.
class DataContainerBase {
 public:
  DataContainerBase(const std::string& name, const int type);
  virtual ~DataContainerBase() = default;

  DataContainerBase()                                    = delete;  //!< Deleted default construct
  DataContainerBase(DataContainerBase&&)                 = delete;  //!< Deleted copy constructor
  DataContainerBase(const DataContainerBase&)            = delete;  //!< Deleted copy constructor
  DataContainerBase& operator=(DataContainerBase&&)      = delete;  //!< Deleted copy assignment
  DataContainerBase& operator=(const DataContainerBase&) = delete;  //!< Deleted copy assignment

  /// \brief Returns value indicating the derived type.
  const int getType() const;
  /// \brief Pure virtual function to prevent this class being instantiated directly.
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace monio
