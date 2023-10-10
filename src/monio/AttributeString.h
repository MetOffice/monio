/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
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

  const std::string& getName() const;
  const std::string& getValue() const;

 private:
  std::string value_;
};
}  // namespace monio
