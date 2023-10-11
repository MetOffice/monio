
/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AttributeBase.h"

namespace monio {
/// \brief Used by Metadata to hold information about a variable read from
/// or to be written to a NetCDF file.
class Variable {
 public:
  Variable(const std::string name, const int type);

  Variable()                           = delete;  //!< Deleted default constructor
  Variable(Variable&&)                 = delete;  //!< Deleted move constructor
  Variable(const Variable&)            = delete;  //!< Deleted copy constructor
  Variable& operator=(Variable&&)      = delete;  //!< Deleted move assignment
  Variable& operator=(const Variable&) = delete;  //!< Deleted copy assignment

  const std::string& getName() const;
  const int getType() const;
  const size_t getTotalSize() const;

  /// \brief Used specifically to retrieve LFRic's "standard_type" variable attributes as the
  ///        closest approximation to a JEDI variable name. These are stored as AttributeString.
  std::string getStrAttr(const std::string& attrName);
  std::shared_ptr<AttributeBase> getAttribute(const std::string& attrName);

  std::vector<std::pair<std::string, size_t>>& getDimensionsMap();
  std::vector<std::string> getDimensionNames();
  std::map<std::string, std::shared_ptr<AttributeBase>>& getAttributes();

  void addDimension(const std::string& name, const size_t size);
  void addAttribute(std::shared_ptr<monio::AttributeBase> attr);

  void deleteDimension(const std::string& dimName);
  void deleteAttribute(const std::string& attrName);

 private:
  std::string name_;
  int type_;
  std::vector<std::pair<std::string, size_t>> dimensions_;
  std::map<std::string, std::shared_ptr<AttributeBase>> attributes_;
};
}  // namespace monio
