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
/// \brief Concrete class for integer attributes of a NetCDF file.
class AttributeInt : public AttributeBase {
 public:
  AttributeInt(const std::string& name, const int value);

  AttributeInt()                               = delete;  //!< Deleted default constructor
  AttributeInt(AttributeInt&&)                 = delete;  //!< Deleted move constructor
  AttributeInt(const AttributeInt&)            = delete;  //!< Deleted copy constructor
  AttributeInt& operator=(AttributeInt&&)      = delete;  //!< Deleted move assignment
  AttributeInt& operator=(const AttributeInt&) = delete;  //!< Deleted copy assignment

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;
  const int getValue() const;

 private:
  int value_;
};
}  // namespace monio
