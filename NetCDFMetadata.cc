/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFMetadata.h"

#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

#include "oops/util/Logger.h"

#include "NetCDFAttributeInt.h"
#include "NetCDFAttributeString.h"

namespace  {
template<typename keyValue, typename typeValue>
std::vector<std::string> extractKeys(std::map<keyValue, typeValue> const& inputMap) {
  std::vector<keyValue> keyVector;
  for (auto const& elementPair : inputMap) {
    keyVector.push_back(elementPair.first);
  }
  return keyVector;
}

template<typename T>
int findInVector(std::vector<T> vector, T searchTerm) {
  typename std::vector<T>::iterator it;
  it = std::find(vector.begin(), vector.end(), searchTerm);
  if (it != vector.end())
    return (it - vector.begin());
  else
    return -1;
}
}  // anonymous namespace

lfriclite::NetCDFMetadata::NetCDFMetadata() {
  oops::Log::debug() << "NetCDFMetadata::NetCDFMetadata()" << std::endl;
}

lfriclite::NetCDFMetadata::~NetCDFMetadata() {
  for (auto it = variables_.begin(); it != variables_.end(); ++it) {
    delete it->second;
  }
  for (auto it = globalAttrs_.begin(); it != globalAttrs_.end(); ++it) {
    delete it->second;
  }
}

bool lfriclite::operator==(const lfriclite::NetCDFMetadata& lhs,
                           const lfriclite::NetCDFMetadata& rhs) {
  // Compare dimensions
  if (lhs.dimensions_.size() == rhs.dimensions_.size()) {
    for (auto lhsIt = lhs.dimensions_.begin(), rhsIt = rhs.dimensions_.begin();
         lhsIt != lhs.dimensions_.end(); ++lhsIt , ++rhsIt)
    {
      if (lhsIt->second != rhsIt->second)
        return false;
    }
  }
  // Compare variables
  if (lhs.variables_.size() == rhs.variables_.size()) {
    for (auto lhsIt = lhs.variables_.begin(), rhsIt = rhs.variables_.begin();
         lhsIt != lhs.variables_.end(); ++lhsIt , ++rhsIt) {
      NetCDFVariable* lhsVariable = lhsIt->second;
      NetCDFVariable* rhsVariable = rhsIt->second;

      std::string lhsName = lhsVariable->getName();
      std::string rhsName = rhsVariable->getName();

      int lhsDataType = lhsVariable->getType();
      int rhsDataType = rhsVariable->getType();

      int lhsTotSize = lhsVariable->getTotalSize();
      int rhsTotSize = rhsVariable->getTotalSize();

      std::vector<std::string> lhsDimsVec = lhsVariable->getDimensionKeys();
      std::vector<std::string> rhsDimsVec = rhsVariable->getDimensionKeys();

      std::map<std::string, size_t>& lhsDimsMap = lhsVariable->getDimensions();
      std::map<std::string, size_t>& rhsDimsMap = rhsVariable->getDimensions();


      if (lhsName == rhsName && lhsDataType == rhsDataType && lhsTotSize == rhsTotSize &&
          lhsDimsMap.size() == rhsDimsMap.size()) {
        // Compare dimensions vectors
        for (auto lhsIt = lhsDimsMap.begin(), rhsIt = rhsDimsMap.begin();
             lhsIt != lhsDimsMap.end(); ++lhsIt , ++rhsIt) {
          if (lhsIt->first != rhsIt->first || lhsIt->second != rhsIt->second)
            return false;
        }
        if (lhsVariable->getAttributes().size() == rhsVariable->getAttributes().size()) {
          for (auto lhsIt = lhsVariable->getAttributes().begin(),
               rhsIt = rhsVariable->getAttributes().begin();
               lhsIt != lhsVariable->getAttributes().end(); ++lhsIt , ++rhsIt)
          {
            NetCDFAttributeBase* lhsVarAttr = lhsIt->second;
            NetCDFAttributeBase* rhsVarAttr = rhsIt->second;

            int lhsVarAttrType = rhsVarAttr->getType();
            int rhsVarAttrType = lhsVarAttr->getType();

            std::string lhsVarAttrName = lhsVarAttr->getName();
            std::string rhsVarAttrName = rhsVarAttr->getName();

            if (lhsVarAttrType == rhsVarAttrType && lhsVarAttrName == rhsVarAttrName) {
              switch (lhsVarAttrType) {
                case lfriclite::ncconsts::dataTypesEnum::eInt: {
                  NetCDFAttributeInt* lhsVarAttrInt =
                      static_cast<NetCDFAttributeInt*>(lhsVarAttr);
                  NetCDFAttributeInt* rhsVarAttrInt =
                      static_cast<NetCDFAttributeInt*>(rhsVarAttr);
                  if (lhsVarAttrInt->getValue() != rhsVarAttrInt->getValue())
                    return false;
                  break;
                }
                case lfriclite::ncconsts::dataTypesEnum::eString: {
                  NetCDFAttributeString* lhsVarAttrStr =
                      static_cast<NetCDFAttributeString*>(lhsVarAttr);
                  NetCDFAttributeString* rhsVarAttrStr =
                      static_cast<NetCDFAttributeString*>(rhsVarAttr);
                  if (lhsVarAttrStr->getValue() != rhsVarAttrStr->getValue())
                    return false;
                  break;
                }
                default:
                  return false;
              }
            } else {
              return false;
            }
          }  // end for( auto lhsIt = lhsVariable->getAttributes().begin() ...
        } else {
          return false;
        }
      } else {
        return false;
      }
    }  // end for (auto lhsIt = lhs.variables_.begin() ...
  } else {
    return false;
  }
  // There is no comparison of global attributes as these are subject to change and (should be)
  // inconsequential to system functioning.
  return true;
}

bool lfriclite::NetCDFMetadata::isDimDefined(const std::string& dimName) {
  oops::Log::debug() << "NetCDFMetadata::isDimDefined()" << std::endl;
  auto it = dimensions_.find(dimName);
  if (it != dimensions_.end())
    return true;
  else
    return false;
}

int lfriclite::NetCDFMetadata::getDimension(const std::string& dimName) {
  oops::Log::debug() << "NetCDFMetadata::getDimension()" << std::endl;
  int dimVal;
  if (isDimDefined(dimName) == true)
    dimVal = dimensions_.at(dimName);
  else
    throw std::runtime_error("NetCDFMetadata::getDimension()> dimension \"" +
                                dimName + "\" not found...");
  return dimVal;
}

lfriclite::NetCDFVariable* lfriclite::NetCDFMetadata::getVariable(const std::string& varName) {
  oops::Log::debug() << "NetCDFMetadata::getVariable()> " << varName << std::endl;
  auto it = variables_.find(varName);
  lfriclite::NetCDFVariable* variable;
  if (it != variables_.end())
    variable = variables_.at(varName);
  else
    throw std::runtime_error("NetCDFMetadata::getVariable()> variable \"" +
                                varName + "\" not found...");

  return variable;
}

std::vector<lfriclite::NetCDFVariable*>
    lfriclite::NetCDFMetadata::getVariables(const std::vector<std::string>& varNames) {
  oops::Log::debug() << "NetCDFMetadata::getVariables()> " << std::endl;
  std::vector<lfriclite::NetCDFVariable*> variables;
  for (const auto& varName : varNames) {
    variables.push_back(getVariable(varName));
  }
  return variables;
}

std::vector<std::string> lfriclite::NetCDFMetadata::getVarStrAttrs(const std::string& attrName) {
  oops::Log::debug() << "NetCDFMetadata::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varNames = getVariableNames();
  return getVarStrAttrs(varNames, attrName);
}

std::vector<std::string> lfriclite::NetCDFMetadata::getVarStrAttrs(
                                                  const std::vector<std::string>& varNames,
                                                  const std::string& attrName) {
  oops::Log::debug() << "NetCDFMetadata::getVarStrAttrs()" << std::endl;
  std::vector<lfriclite::NetCDFVariable*> variables = getVariables(varNames);
  std::vector<std::string> varStrAttrs;
  for (const auto& var : variables) {
      std::string attr = var->getStrAttr(attrName);
      varStrAttrs.push_back(attr);
  }
  if (varNames.size() != varStrAttrs.size())
    throw std::runtime_error("NetCDFMetadata::getVarStrAttrs()> "
        "Unmatched number of variables and attributes...");

  return varStrAttrs;
}

void lfriclite::NetCDFMetadata::addDimension(const std::string& dimName, const int value) {
  oops::Log::debug() << "NetCDFMetadata::addDimension()" << std::endl;
  auto it = dimensions_.find(dimName);
  if (it == dimensions_.end())
    dimensions_.insert({dimName, value});
  else
    throw std::runtime_error("NetCDFMetadata::addDimension()> multiple definitions of \"" +
                                dimName + "\"...");
}


void lfriclite::NetCDFMetadata::addGlobalAttr(const std::string& attrName,
                                               NetCDFAttributeBase* attr) {
  oops::Log::debug() << "NetCDFMetadata::addGlobalAttr()" << std::endl;
  auto it = globalAttrs_.find(attrName);
  if (it == globalAttrs_.end())
    globalAttrs_.insert({attrName, attr});
  else
    throw std::runtime_error("NetCDFMetadata::addGlobalAttr()> multiple definitions of \"" +
                             attrName + "\"...");
}

void lfriclite::NetCDFMetadata::addVariable(const std::string& varName,
                                            NetCDFVariable* var) {
  oops::Log::debug() << "NetCDFMetadata::addVariable()" << std::endl;
  auto it = variables_.find(varName);
  if (it == variables_.end())
    variables_.insert({varName, var});
  else
    throw std::runtime_error("NetCDFMetadata::addVariable()> multiple definitions of \"" +
                                varName + "\"...");
}

std::vector<std::string> lfriclite::NetCDFMetadata::getDimensionNames() {
  oops::Log::debug() << "NetCDFMetadata::getDimensionNames()" << std::endl;
  return extractKeys(dimensions_);
}

std::vector<std::string> lfriclite::NetCDFMetadata::getVariableNames() {
  oops::Log::debug() << "NetCDFMetadata::getVariableNames()" << std::endl;
  return extractKeys(variables_);
}

std::vector<std::string> lfriclite::NetCDFMetadata::getGlobalAttrNames() {
  oops::Log::debug() << "NetCDFMetadata::getGlobalAttrNames()" << std::endl;
  return extractKeys(globalAttrs_);
}

std::map<std::string, int>& lfriclite::NetCDFMetadata::getDimensionsMap() {
  oops::Log::debug() << "NetCDFMetadata::getDimensionsMap()" << std::endl;
  return dimensions_;
}
std::map<std::string, lfriclite::NetCDFVariable*>& lfriclite::NetCDFMetadata::getVariablesMap() {
  oops::Log::debug() << "NetCDFMetadata::getVariablesMap()" << std::endl;
  return variables_;
}

std::map<std::string, lfriclite::NetCDFAttributeBase*>&
                      lfriclite::NetCDFMetadata::getGlobalAttrsMap() {
  oops::Log::debug() << "NetCDFMetadata::getGlobalAttrsMap()" << std::endl;
  return globalAttrs_;
}

void lfriclite::NetCDFMetadata::removeAllButTheseVariables(
    const std::vector<std::string>& varNames) {
  oops::Log::debug() << "NetCDFMetadata::removeAllButTheseVariables()" << std::endl;
  std::vector<std::string> variableKeys = extractKeys(variables_);
  for (const std::string& variableKey : variableKeys) {
    int index = findInVector(varNames, variableKey);
    if (index == -1)
      deleteVariable(variableKey);
  }
}

void lfriclite::NetCDFMetadata::deleteDimension(const std::string& dimName) {
  oops::Log::debug() << "NetCDFMetadata::deleteDimension()" << std::endl;
  std::map<std::string, int>::iterator itDim = dimensions_.find(dimName);
  if (itDim == dimensions_.end()) {
    throw std::runtime_error("NetCDFMetadata::deleteDimension()> Dimension \"" +
                             dimName + "\" not found...");
  } else {
    dimensions_.erase(dimName);
  }
  for (const auto& varPair : variables_) {
    varPair.second->deleteDimension(dimName);
  }
}

void lfriclite::NetCDFMetadata::deleteVariable(const std::string& varName) {
  oops::Log::debug() << "NetCDFMetadata::deleteVariable()" << std::endl;
  std::map<std::string, NetCDFVariable*>::iterator it = variables_.find(varName);
  if (it == variables_.end()) {
    throw std::runtime_error("NetCDFMetadata::deleteVariable()> Variable \"" +
                             varName + "\" not found...");
  } else {
    NetCDFVariable* netCDFVar = it->second;
    delete netCDFVar;
    variables_.erase(varName);
  }
}

void lfriclite::NetCDFMetadata::print() {
  oops::Log::debug() << "dimensions:" << std::endl;
  printMap(dimensions_);
  oops::Log::debug() << "variables:" << std::endl;
  printVariables();
  oops::Log::debug() << "attributes:" << std::endl;
  printGlobalAttrs();
}

void lfriclite::NetCDFMetadata::printVariables() {
  for (auto const& var : variables_) {
    NetCDFVariable* netCDFVar = var.second;
    oops::Log::debug() << lfriclite::ncconsts::kTabSpace <<
                         lfriclite::ncconsts::kDataTypeNames[netCDFVar->getType()] <<
                         " " << netCDFVar->getName();

    std::vector<std::string> varDims = netCDFVar->getDimensionKeys();

    if (varDims.size() > 0)
    {
      oops::Log::debug() << "(";
      for (auto it = varDims.begin(); it != varDims.end() - 1; ++it) {
        oops::Log::debug() << *it << ", ";
      }
      oops::Log::debug() << *(varDims.end() - 1) << ")" << std::endl;
    } else {
      oops::Log::debug() << std::endl;
    }

    std::map<std::string, NetCDFAttributeBase*> varAttrsMap = netCDFVar->getAttributes();
    for (auto const& varAttrPair : varAttrsMap) {
      NetCDFAttributeBase* netCDFAttr = varAttrPair.second;

      oops::Log::debug() << lfriclite::ncconsts::kTabSpace <<
                           lfriclite::ncconsts::kTabSpace <<
                           netCDFVar->getName() << ":" <<
                           netCDFAttr->getName() << " = ";
      int dataType = netCDFAttr->getType();

      switch (dataType) {
        case lfriclite::ncconsts::dataTypesEnum::eInt: {
          NetCDFAttributeInt* netCDFAttrInt = static_cast<NetCDFAttributeInt*>(netCDFAttr);
          oops::Log::debug() << netCDFAttrInt->getValue() << " ;" << std::endl;
          break;
        }
        case lfriclite::ncconsts::dataTypesEnum::eString: {
          NetCDFAttributeString* netCDFAttrStr = static_cast<NetCDFAttributeString*>(netCDFAttr);
          oops::Log::debug() << std::quoted(netCDFAttrStr->getValue()) << std::endl;
          break;
        }
        default:
          throw std::runtime_error("NetCDFMetadata::printGlobalAttrs()> "
                                   "Data type not coded for...");
      }
    }
  }
}

void lfriclite::NetCDFMetadata::printGlobalAttrs() {
  for (const auto& globAttrPair : globalAttrs_) {
    oops::Log::debug() << lfriclite::ncconsts::kTabSpace << globAttrPair.first;
    NetCDFAttributeBase* globalAttr = globAttrPair.second;
    int type = globalAttr->getType();
    switch (type) {
      case lfriclite::ncconsts::dataTypesEnum::eInt: {
        NetCDFAttributeInt* globalAttrInt = static_cast<NetCDFAttributeInt*>(globalAttr);
        oops::Log::debug() << globalAttrInt->getValue() << " ;" << std::endl;
        break;
      }
      case lfriclite::ncconsts::dataTypesEnum::eString: {
        NetCDFAttributeString* globAttrStr = static_cast<NetCDFAttributeString*>(globalAttr);
        oops::Log::debug() << " = " << std::quoted(globAttrStr->getValue()) << " ;"  << std::endl;
        break;
      }
      default:
        throw std::runtime_error("NetCDFMetadata::printGlobalAttrs()> Data type not coded for...");
    }
  }
}

template<typename T>
void lfriclite::NetCDFMetadata::printMap(std::map<std::string, T>& map) {
  for (const auto& entry : map) {
    oops::Log::debug() << lfriclite::ncconsts::kTabSpace << entry.first <<
                           " = " << entry.second << " ;" << std::endl;
  }
}

template void lfriclite::NetCDFMetadata::printMap<int>(
    std::map<std::string, int>& map);
template void lfriclite::NetCDFMetadata::printMap<std::string>(
    std::map<std::string, std::string>& map);

