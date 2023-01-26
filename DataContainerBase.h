/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

namespace monio {
/// \brief Pure abstract base class for data read from or to be written to a NetCDF file
class DataContainerBase {
 public:
  DataContainerBase(const std::string& name, const int type);
  virtual ~DataContainerBase() = default;

  DataContainerBase()                         = delete;             //!< Deleted default construct
  DataContainerBase(const DataContainerBase&) = delete;             //!< Deleted copy constructor
  DataContainerBase(DataContainerBase&&)      = delete;             //!< Deleted move constructor

  DataContainerBase& operator=(const DataContainerBase&) = delete;  //!< Deleted copy assignment
  DataContainerBase& operator=(DataContainerBase&&)      = delete;  //!< Deleted move assignment

  const int getType() const;
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace lfriclite
