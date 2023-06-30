/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "Variable.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "AttributeInt.h"
#include "AttributeString.h"
#include "Constants.h"
#include "Utils.h"

monio::Variable::Variable(const std::string name, const int type):
  name_(name), type_(type) {}

const std::string& monio::Variable::getName() const {
  return name_;
}

const int monio::Variable::getType() const {
  return type_;
}

const size_t monio::Variable::getTotalSize() const {
  size_t totalSize = 1;
  for (const auto& dim : dimensions_) {
    totalSize *= dim.second;
  }
  return totalSize;
}

std::vector<int> monio::Variable::getDimensionsVec() {
  std::vector<int> dimensionsVec;
  for (const auto& dim : dimensions_) {
    dimensionsVec.push_back(dim.second);
  }
  return dimensionsVec;
}

std::vector<std::pair<std::string, size_t>>& monio::Variable::getDimensionsMap() {
  return dimensions_;
}

std::vector<std::string> monio::Variable::getDimensionNames() {
  std::vector<std::string> dimNames;
  for (auto const& dimPair : dimensions_) {
    dimNames.push_back(dimPair.first);
  }
  return dimNames;
}


std::map<std::string, std::shared_ptr<monio::AttributeBase>>&
                                      monio::Variable::getAttributes() {
  return attributes_;
}

std::shared_ptr<monio::AttributeBase>
                monio::Variable::getAttribute(const std::string& attrName) {
  if (attributes_.find(attrName) != attributes_.end()) {
    return attributes_.at(attrName);
  } else {
    throw std::runtime_error("Variable::getAttribute()> Attribute \"" +
                                    attrName + "\" not found...");
  }
}

std::string monio::Variable::getStrAttr(const std::string& attrName) {
  if (attributes_.find(attrName) != attributes_.end()) {
    std::shared_ptr<monio::AttributeBase> attr = attributes_.at(attrName);

    std::string value;
    if (attr->getType() == constants::eString) {
      std::shared_ptr<monio::AttributeString> attrStr =
                        std::dynamic_pointer_cast<monio::AttributeString>(attr);
      value = attrStr->getValue();
      return value;
    } else {
      // Does not currently handle AttributeInt types. Used specifically to retrieve LFRic's
      // "standard_type" variable attributes as the closest approximation to a JEDI variable name.
      // These are stored as AttributeString.
      throw std::runtime_error("Variable::getAttribute()> "
          "Variable attribute data type not coded for...");
    }
  } else {
    throw std::runtime_error("Variable::getAttribute()> Attribute \"" +
                                    attrName + "\" not found...");
  }
}

void monio::Variable::addDimension(const std::string& dimName, const size_t size) {
  auto it = std::find_if(dimensions_.begin(), dimensions_.end(),
      [&](const std::pair<std::string, size_t>& element) { return element.first == dimName; });

  if (it == dimensions_.end()) {
    dimensions_.push_back(std::make_pair(dimName, size));
  } else {
      throw std::runtime_error("Variable::addDimension()> Dimension \"" +
                                    dimName + "\" already defined...");
  }
}

void monio::Variable::addAttribute(std::shared_ptr<monio::AttributeBase> attr) {
  std::string attrName = attr->getName();
  auto it = attributes_.find(attrName);
  if (it == attributes_.end()) {
    attributes_.insert(std::make_pair(attrName, attr));
  } else {
    throw std::runtime_error("Variable::addAttribute()>  multiple definitions of \"" +
                                attrName + "\"...");
  }
}

void monio::Variable::deleteDimension(const std::string& dimName) {
  auto it = std::find_if(dimensions_.begin(), dimensions_.end(),
      [&](const std::pair<std::string, size_t>& element) {
        return element.first == dimName;
      });
  if (it != dimensions_.end()) {
    dimensions_.erase(it);
  }
}

void monio::Variable::deleteAttribute(const std::string& attrName) {
  auto it = attributes_.find(attrName);
  if (it != attributes_.end()) {
    attributes_.erase(attrName);
  } else {
      throw std::runtime_error("Variable::deleteAttribute()> Attribute \"" +
                                    attrName + "\" does not exist...");
  }
}

size_t monio::Variable::getDimension(const std::string& dimName) {
  auto it = std::find_if(dimensions_.begin(), dimensions_.end(),
      [&](const std::pair<std::string, size_t>& element) {
        return element.first == dimName;
      });
  if (it != dimensions_.end()) {
    return it->second;
  } else {
      throw std::runtime_error("Variable::getDimension()> Dimension \"" +
                                    dimName + "\" does not exist...");
  }
}

size_t monio::Variable::findDimensionSize(const std::string& dimSearchTerm) {
  size_t dimSize = 1;
  for (auto it = dimensions_.begin(); it != dimensions_.end(); ++it) {
    std::string name = it->first;
    size_t index = name.find(dimSearchTerm);
    if (index != std::string::npos) {
      dimSize = it->second;
      break;
    }
  }
  return dimSize;
}
