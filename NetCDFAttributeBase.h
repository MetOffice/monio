/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

namespace lfriclite {
/// \brief Pure abstract base class for attributes of a NetCDF file
class NetCDFAttributeBase {
 public:
  NetCDFAttributeBase(const std::string& name, const int type);
  virtual ~NetCDFAttributeBase() = default;

  NetCDFAttributeBase()                           = delete;  //!< Deleted default constructor
  NetCDFAttributeBase(const NetCDFAttributeBase&) = delete;  //!< Deleted copy constructor
  NetCDFAttributeBase(NetCDFAttributeBase&&)      = delete;  //!< Deleted move constructor

  NetCDFAttributeBase& operator=(const NetCDFAttributeBase&) = delete;  //!< Deleted copy assign
  NetCDFAttributeBase& operator=(NetCDFAttributeBase&&) = delete;       //!< Deleted move assign

  const int getType() const;
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace lfriclite
