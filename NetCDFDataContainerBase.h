/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

namespace lfriclite {
/// \brief Pure abstract base class for data read from or to be written to a NetCDF file
class NetCDFDataContainerBase {
 public:
  NetCDFDataContainerBase(const std::string& name, const int type);
  virtual ~NetCDFDataContainerBase() = default;

  NetCDFDataContainerBase()                           = delete;  //!< Deleted default constructor
  NetCDFDataContainerBase(const NetCDFDataContainerBase&) = delete;  //!< Deleted copy constructor
  NetCDFDataContainerBase(NetCDFDataContainerBase&&)      = delete;  //!< Deleted move constructor

  NetCDFDataContainerBase& operator=(const NetCDFDataContainerBase&) = delete;  //!< Deleted copy a
  NetCDFDataContainerBase& operator=(NetCDFDataContainerBase&&) = delete;  //!< Deleted move assign

  const int getType() const;
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace lfriclite
