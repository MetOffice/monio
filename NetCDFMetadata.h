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

#include "NetCDFConstants.h"
#include "NetCDFVariable.h"

namespace lfriclite {
/// \brief Holds metadata read from or to be written to a NetCDF file
class NetCDFMetadata {
 public:
  NetCDFMetadata();
  ~NetCDFMetadata();

  NetCDFMetadata(const NetCDFMetadata&) = delete;  //!< Deleted copy constructor
  NetCDFMetadata(NetCDFMetadata&&)      = delete;  //!< Deleted move constructor

  NetCDFMetadata& operator=(const NetCDFMetadata&) = delete;  //!< Deleted copy assign
  NetCDFMetadata& operator=(NetCDFMetadata&&) = delete;       //!< Deleted move assign

  friend bool operator==(const NetCDFMetadata& lhs,
                         const NetCDFMetadata& rhs);

  bool isDimDefined(const std::string& dimName);
  int getDimension(const std::string& dimName);
  NetCDFVariable* getVariable(const std::string& varName);

  std::vector<NetCDFVariable*> getVariables(const std::vector<std::string>& varNames);
  std::vector<std::string> getVarStrAttrs(const std::string& attrName);
  std::vector<std::string> getVarStrAttrs(const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  void addDimension(const std::string& dimName, const int value);
  void addGlobalAttr(const std::string& attrName, NetCDFAttributeBase* attr);
  void addVariable(const std::string& varName, NetCDFVariable* var);

  std::vector<std::string> getDimensionNames();
  std::vector<std::string> getVariableNames();
  std::vector<std::string> getGlobalAttrNames();

  std::map<std::string, int>& getDimensionsMap();
  std::map<std::string, NetCDFVariable*>& getVariablesMap();
  std::map<std::string, NetCDFAttributeBase*>& getGlobalAttrsMap();

  void removeAllButTheseVariables(const std::vector<std::string>& varNames);

  void deleteDimension(const std::string& dimName);
  void deleteVariable(const std::string& varName);

  void print();

 private:
  void printVariables();
  void printGlobalAttrs();
  template<typename T> void printMap(std::map<std::string, T>& map);

  std::map<std::string, int> dimensions_;
  std::map<std::string, NetCDFAttributeBase*> globalAttrs_;
  std::map<std::string, NetCDFVariable*> variables_;
};

bool operator==(const NetCDFMetadata& lhs,
                const NetCDFMetadata& rhs);
}  // namespace lfriclite
