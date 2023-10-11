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
/// \brief Concrete class for string attributes of a NetCDF file.
class AttributeString : public AttributeBase {
 public:
  AttributeString(const std::string& name, const std::string& value);

  AttributeString()                                  = delete;  //!< Deleted default constructor
  AttributeString(AttributeString&&)                 = delete;  //!< Deleted copy constructor
  AttributeString(const AttributeString&)            = delete;  //!< Deleted copy constructor
  AttributeString& operator=(AttributeString&&)      = delete;  //!< Deleted copy assignment
  AttributeString& operator=(const AttributeString&) = delete;  //!< Deleted copy assignment

  /// \brief Implemented by contract from base class.
  const std::string& getName() const;
  const std::string& getValue() const;

 private:
  std::string value_;
};
}  // namespace monio
