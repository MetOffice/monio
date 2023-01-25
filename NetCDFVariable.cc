/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFVariable.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "NetCDFConstants.h"
#include "NetCDFAttributeInt.h"
#include "NetCDFAttributeString.h"

namespace {
template <typename key, typename value>
std::vector<key> getVectorOfKeys(std::map<key, value>& map) {
  std::vector<key> keys;
  for (auto const& entry : map) {
    keys.push_back(entry.first);
  }
  return keys;
}
}  // anonymous namespace

lfriclite::NetCDFVariable::NetCDFVariable(const std::string name, const int type):
  name_(name), type_(type) {}

lfriclite::NetCDFVariable::~NetCDFVariable() {
  for (auto it = attributes_.begin(); it != attributes_.end(); ++it) {
    delete it->second;
  }
}

const std::string& lfriclite::NetCDFVariable::getName() const {
  return name_;
}

const int lfriclite::NetCDFVariable::getType() const {
  return type_;
}

const size_t lfriclite::NetCDFVariable::getTotalSize() const {
  return totalSize_;
}

std::map<std::string, size_t>& lfriclite::NetCDFVariable::getDimensions() {
  return dimensions_;
}

std::vector<std::string> lfriclite::NetCDFVariable::getDimensionKeys() {
  return getVectorOfKeys(dimensions_);
}

std::map<std::string, lfriclite::NetCDFAttributeBase*>&
    lfriclite::NetCDFVariable::getAttributes() {
  return attributes_;
}

lfriclite::NetCDFAttributeBase* lfriclite::NetCDFVariable::getAttribute(
                                                  const std::string& attrName) {
  if (attributes_.find(attrName) != attributes_.end()) {
    return attributes_[attrName];
  } else {
    throw std::runtime_error("NetCDFVariable::getAttribute()> Attribute \"" +
                                    attrName + "\" not found...");
  }
}

std::string lfriclite::NetCDFVariable::getStrAttr(const std::string& attrName) {
  if (attributes_.find(attrName) != attributes_.end()) {
    NetCDFAttributeBase* attr = attributes_[attrName];

    std::string value;
    if (attr->getType() == lfriclite::ncconsts::eString) {
      NetCDFAttributeString* attrStr = static_cast<NetCDFAttributeString*>(attr);
      value = attrStr->getValue();
      return value;
    } else {
      // Does not currently handle NetCDFAttributeInt types. Used specifically to retrieve LFRic's
      // "standard_type" variable attributes as the closest approximation to a JEDI variable name.
      // These are stored as NetCDFAttributeString.
      throw std::runtime_error("NetCDFVariable::getAttribute()> "
          "Variable attribute data type not coded for...");
    }
  } else {
    throw std::runtime_error("NetCDFVariable::getAttribute()> Attribute \"" +
                                    attrName + "\" not found...");
  }
}

void lfriclite::NetCDFVariable::addDimension(const std::string& name, const size_t size) {
  if (dimensions_.find(name) == dimensions_.end()) {
    dimensions_.insert(std::make_pair(name, size));
  } else {
    throw std::runtime_error("NetCDFVariable::addDimension()> Dimension \"" +
                                  name + "\" already defined...");
  }
}

void lfriclite::NetCDFVariable::addAttribute(lfriclite::NetCDFAttributeBase* attr) {
  std::string attrName = attr->getName();
  auto it = attributes_.find(attrName);
  if (it == attributes_.end())
    attributes_.insert(std::make_pair(attrName, attr));
  else
    throw std::runtime_error("NetCDFVariable::addAttribute()>  multiple definitions of \"" +
                                attrName + "\"...");
}

void lfriclite::NetCDFVariable::setTotalSize(const size_t totalSize) {
  totalSize_ = totalSize;
}

void lfriclite::NetCDFVariable::deleteDimension(const std::string& dimName) {
  std::map<std::string, size_t>::iterator it = dimensions_.find(dimName);
  if (it != dimensions_.end()) {
    dimensions_.erase(dimName);
  }
}

void lfriclite::NetCDFVariable::deleteAttribute(const std::string& attrName) {
  std::map<std::string, NetCDFAttributeBase*>::iterator it = attributes_.find(attrName);
  if (it != attributes_.end()) {
    NetCDFAttributeBase* netCDFAttr = it->second;
    delete netCDFAttr;
    attributes_.erase(attrName);
  }
}

size_t lfriclite::NetCDFVariable::findDimension(const std::string& dimSearchTerm) {
  size_t numLevels = 1;
  for (auto it = dimensions_.begin(); it != dimensions_.end(); ++it) {
    std::string name = it->first;
    size_t index = name.find(dimSearchTerm);
    if (index != std::string::npos) {
      numLevels = it->second;
      break;
    }
  }
  return numLevels;
}
