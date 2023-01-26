/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

#include "AttributeBase.h"

namespace monio {
/// \brief Concrete class for integer attributes of a NetCDF file
class AttributeInt : public AttributeBase {
 public:
  AttributeInt(const std::string& name, const int value);

  AttributeInt()                               = delete;  //!< Deleted default constructor
  AttributeInt(const AttributeInt&)            = delete;  //!< Deleted copy constructor
  AttributeInt(AttributeInt&&)                 = delete;  //!< Deleted move constructor

  AttributeInt& operator=(const AttributeInt&) = delete;  //!< Deleted copy assignment
  AttributeInt& operator=(AttributeInt&&)      = delete;  //!< Deleted move assignment

  const std::string& getName() const;
  const int getValue() const;

 private:
  int value_;
};
}  // namespace lfriclite
