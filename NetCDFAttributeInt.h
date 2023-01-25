/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

#include "NetCDFAttributeBase.h"

namespace lfriclite {
/// \brief Concrete class for integer attributes of a NetCDF file
class NetCDFAttributeInt : public NetCDFAttributeBase {
 public:
  NetCDFAttributeInt(const std::string& name, const int value);

  NetCDFAttributeInt()                      = delete;  //!< Deleted default constructor
  NetCDFAttributeInt(const NetCDFAttributeInt&) = delete;  //!< Deleted copy constructor
  NetCDFAttributeInt(NetCDFAttributeInt&&)      = delete;  //!< Deleted move constructor

  NetCDFAttributeInt& operator=(const NetCDFAttributeInt&) = delete;  //!< Deleted copy assign
  NetCDFAttributeInt& operator=(NetCDFAttributeInt&&) = delete;       //!< Deleted move assign

  const std::string& getName() const;
  const int getValue() const;

 private:
  int value_;
};
}  // namespace lfriclite
