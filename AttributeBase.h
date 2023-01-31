/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>

namespace monio {
/// \brief Pure abstract base class for attributes of a NetCDF file
class AttributeBase {
 public:
  AttributeBase(const std::string& name, const int type);
  virtual ~AttributeBase() = default;

  AttributeBase()                                = delete;  //!< Deleted default constructor
  AttributeBase(const AttributeBase&)            = delete;  //!< Deleted copy constructor
  AttributeBase(AttributeBase&&)                 = delete;  //!< Deleted move constructor

  AttributeBase& operator=(const AttributeBase&) = delete;  //!< Deleted copy assignment
  AttributeBase& operator=(AttributeBase&&)      = delete;  //!< Deleted move assignment

  const int getType() const;
  virtual const std::string& getName() const = 0;

 protected:
  std::string name_;
  int type_;
};
}  // namespace monio
