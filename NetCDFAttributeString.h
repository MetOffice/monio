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
/// \brief Concrete class for string attributes of a NetCDF file
class NetCDFAttributeString : public NetCDFAttributeBase {
 public:
  NetCDFAttributeString(const std::string& name, const std::string& value);

  NetCDFAttributeString()                      = delete;  //!< Deleted default constructor
  NetCDFAttributeString(const NetCDFAttributeString&) = delete;  //!< Deleted copy constructor
  NetCDFAttributeString(NetCDFAttributeString&&)      = delete;  //!< Deleted move constructor

  NetCDFAttributeString& operator=(const NetCDFAttributeString&) = delete;  //!< Deleted copy assig
  NetCDFAttributeString& operator=(NetCDFAttributeString&&) = delete;    //!< Deleted move assignme

  const std::string& getName() const;
  const std::string& getValue() const;

 private:
  std::string value_;
};
}  // namespace lfriclite
