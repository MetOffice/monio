/*
 * (C) Crown Copyright 2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

#include "AttributeBase.h"

namespace monio {
/// \brief Concrete class for integer attributes of a NetCDF file
class AttributeDouble : public AttributeBase {
 public:
  AttributeDouble(const std::string& name, const double value);

  AttributeDouble()                                  = delete;  //!< Deleted default constructor
  AttributeDouble(AttributeDouble&&)                 = delete;  //!< Deleted move constructor
  AttributeDouble(const AttributeDouble&)            = delete;  //!< Deleted copy constructor
  AttributeDouble& operator=(AttributeDouble&&)      = delete;  //!< Deleted move assignment
  AttributeDouble& operator=(const AttributeDouble&) = delete;  //!< Deleted copy assignment

  const std::string& getName() const;
  const double getValue() const;

 private:
  double value_;
};
}  // namespace monio
