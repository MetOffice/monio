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

namespace monio {
/// \brief Pure abstract base class for attributes of a NetCDF file.
class AttributeBase {
 public:
  AttributeBase(const std::string& name, const int type);
  virtual ~AttributeBase() = default;

  AttributeBase()                                 = delete;  //!< Deleted default constructor
  AttributeBase(AttributeBase&&)                  = delete;  //!< Deleted move constructor
  AttributeBase(const AttributeBase&)             = delete;  //!< Deleted copy constructor
  AttributeBase& operator=(AttributeBase&&)       = delete;  //!< Deleted move assignment
  AttributeBase& operator=(const AttributeBase&)  = delete;  //!< Deleted copy assignment

  /// \brief Returns value indicating the derived type.
  const int getType() const;
  /// \brief Pure virtual function to prevent this class being instantiated directly.
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace monio
