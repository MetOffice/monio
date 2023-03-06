/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Constants.h"
#include "Variable.h"

namespace monio {
/// \brief Holds metadata read from or to be written to a NetCDF file
class Metadata {
 public:
  Metadata();

  Metadata(const Metadata&)            = delete;  //!< Deleted copy constructor
  Metadata& operator=(const Metadata&) = delete;  //!< Deleted copy assign

  friend bool operator==(const Metadata& lhs,
                         const Metadata& rhs);

  bool isDimDefined(const std::string& dimName);
  int getDimension(const std::string& dimName);

  std::string getDimensionName(const int dimValue);

  std::shared_ptr<Variable> getVariable(const std::string& varName);
  const std::shared_ptr<Variable> getVariable(const std::string& varName) const;

  std::vector<std::shared_ptr<Variable>> getVariables(const std::vector<std::string>& varNames);
  std::vector<std::string> getVarStrAttrs(const std::string& attrName);
  std::vector<std::string> getVarStrAttrs(const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  void addDimension(const std::string& dimName, const int value);
  void addGlobalAttr(const std::string& attrName, std::shared_ptr<AttributeBase> attr);
  void addVariable(const std::string& varName, std::shared_ptr<Variable> var);

  std::vector<std::string> getDimensionNames();
  std::vector<std::string> getGlobalAttrNames();
  std::vector<std::string> getVariableNames();

  std::map<std::string, int>& getDimensionsMap();
  std::map<std::string, std::shared_ptr<AttributeBase>>& getGlobalAttrsMap();
  std::map<std::string, std::shared_ptr<Variable>>& getVariablesMap();

  const std::map<std::string, int>& getDimensionsMap() const;
  const std::map<std::string, std::shared_ptr<AttributeBase>>& getGlobalAttrsMap() const;
  const std::map<std::string, std::shared_ptr<Variable>>& getVariablesMap() const;

  void removeAllButTheseVariables(const std::vector<std::string>& varNames);

  void deleteDimension(const std::string& dimName);
  void deleteVariable(const std::string& varName);

  void print();

 private:
  void printVariables();
  void printGlobalAttrs();
  template<typename T> void printMap(const std::map<std::string, T>& map);

  std::map<std::string, int> dimensions_;
  std::map<std::string, std::shared_ptr<AttributeBase>> globalAttrs_;
  std::map<std::string, std::shared_ptr<Variable>> variables_;
};

bool operator==(const Metadata& lhs,
                const Metadata& rhs);
}  // namespace monio
