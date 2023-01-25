
/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

#include "NetCDFAttributeBase.h"

namespace lfriclite {
/// \brief Used by NetCDFMetadata to hold information about a variable read from
/// or to be written to a NetCDF file.
class NetCDFVariable {
 public:
  NetCDFVariable(const std::string name, const int type);
  ~NetCDFVariable();

  NetCDFVariable()                      = delete;  //!< Deleted default constructor
  NetCDFVariable(const NetCDFVariable&) = delete;  //!< Deleted copy constructor
  NetCDFVariable(NetCDFVariable&&)      = delete;  //!< Deleted move constructor

  NetCDFVariable& operator=(const NetCDFVariable&) = delete;  //!< Deleted copy assign
  NetCDFVariable& operator=(NetCDFVariable&&) = delete;       //!< Deleted move assign

  const std::string& getName() const;
  const int getType() const;
  const size_t getTotalSize() const;

  std::string getStrAttr(const std::string& attrName);
  NetCDFAttributeBase* getAttribute(const std::string& attrName);

  std::map<std::string, size_t>& getDimensions();
  std::vector<std::string> getDimensionKeys();
  std::map<std::string, NetCDFAttributeBase*>& getAttributes();

  size_t findDimension(const std::string& dimSearchTerm);

  void setTotalSize(const size_t totalSize);
  void addDimension(const std::string& name, const size_t size);
  void addAttribute(NetCDFAttributeBase* attr);

  void deleteDimension(const std::string& dimName);
  void deleteAttribute(const std::string& attrName);

 private:
  std::string name_;
  int type_;
  size_t totalSize_;
  std::map<std::string, size_t> dimensions_;
  std::map<std::string, NetCDFAttributeBase*> attributes_;
};
}  // namespace lfriclite
