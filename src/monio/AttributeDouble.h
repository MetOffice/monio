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

#include "AttributeBase.h"

namespace monio {
/// \brief Concrete class for double attributes of a NetCDF file.
class AttributeDouble : public AttributeBase {
 public:
  AttributeDouble(const std::string& name, const double value);

  AttributeDouble()                                  = delete;  //!< Deleted default constructor
  AttributeDouble(AttributeDouble&&)                 = delete;  //!< Deleted move constructor
  AttributeDouble(const AttributeDouble&)            = delete;  //!< Deleted copy constructor
  AttributeDouble& operator=(AttributeDouble&&)      = delete;  //!< Deleted move assignment
  AttributeDouble& operator=(const AttributeDouble&) = delete;  //!< Deleted copy assignment

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;
  const double getValue() const;

 private:
  double value_;
};
}  // namespace monio
