/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFData.h"

#include <stdexcept>
#include <vector>

#include "NetCDFConstants.h"
#include "NetCDFDataContainerDouble.h"
#include "NetCDFDataContainerFloat.h"
#include "NetCDFDataContainerInt.h"

#include "oops/util/Logger.h"

namespace {
template<typename T> bool compareData(std::vector<T>& lhsVec, std::vector<T>& rhsVec) {
  if (lhsVec.size() == rhsVec.size()) {
    for (auto lhsIt = lhsVec.begin(), rhsIt = rhsVec.begin();
         lhsIt != lhsVec.end(); ++lhsIt , ++rhsIt) {
      if (*lhsIt != *rhsIt)
        return false;
    }
    return true;
  }
  return false;
}
}  // anonymous namespace

lfriclite::NetCDFData::NetCDFData() {}

lfriclite::NetCDFData::~NetCDFData() {
  for (auto it = dataContainers_.begin(); it != dataContainers_.end(); ++it) {
    delete it->second;
  }
}

bool lfriclite::operator==(const lfriclite::NetCDFData& lhs, const lfriclite::NetCDFData& rhs) {
  if (lhs.dataContainers_.size() == rhs.dataContainers_.size()) {
    for (auto lhsIt = lhs.dataContainers_.begin(), rhsIt = rhs.dataContainers_.begin();
         lhsIt != lhs.dataContainers_.end(); ++lhsIt , ++rhsIt)
    {
      NetCDFDataContainerBase* lhsDataContainer = lhsIt->second;
      NetCDFDataContainerBase* rhsDataContainer = rhsIt->second;

      int lhsDataType = lhsDataContainer->getType();
      int rhsDataType = rhsDataContainer->getType();

      std::string lhsName = lhsDataContainer->getName();
      std::string rhsName = rhsDataContainer->getName();

      if (lhsDataType == rhsDataType && lhsName == rhsName) {
        switch (lhsDataType) {
          case lfriclite::ncconsts::dataTypesEnum::eDouble: {
            NetCDFDataContainerDouble* lhsDataContainerDouble =
              static_cast<NetCDFDataContainerDouble*>(lhsDataContainer);
            NetCDFDataContainerDouble* rhsDataContainerDouble =
              static_cast<NetCDFDataContainerDouble*>(rhsDataContainer);
            if (compareData(lhsDataContainerDouble->getData(),
                          rhsDataContainerDouble->getData()) == false)
              return false;
            break;
          }
          case lfriclite::ncconsts::dataTypesEnum::eFloat: {
            NetCDFDataContainerFloat* lhsDataContainerFloat =
              static_cast<NetCDFDataContainerFloat*>(lhsDataContainer);
            NetCDFDataContainerFloat* rhsDataContainerFloat =
              static_cast<NetCDFDataContainerFloat*>(rhsDataContainer);
            if (compareData(lhsDataContainerFloat->getData(),
                          rhsDataContainerFloat->getData()) == false)
              return false;
            break;
          }
          case lfriclite::ncconsts::dataTypesEnum::eInt: {
            NetCDFDataContainerInt* lhsDataContainerInt =
              static_cast<NetCDFDataContainerInt*>(lhsDataContainer);
            NetCDFDataContainerInt* rhsDataContainerInt =
              static_cast<NetCDFDataContainerInt*>(rhsDataContainer);
            if (compareData(lhsDataContainerInt->getData(),
                          rhsDataContainerInt->getData()) == false)
              return false;
            break;
          }
          default:
            return false;
        }
      } else {
        return false;
      }
    }
  } else {
    return false;
  }
  return true;
}

void lfriclite::NetCDFData::addContainer(NetCDFDataContainerBase* container) {
  oops::Log::debug() << "NetCDFData::addContainer()" << std::endl;
  const std::string& name = container->getName();
  auto it = dataContainers_.find(name);
  if (it == dataContainers_.end())
    dataContainers_.insert({name, container});
  else
    throw std::runtime_error("NetCDFData::addContainer()> multiple "
        "definitions of \"" + name + "\"...");
}

lfriclite::NetCDFDataContainerBase* lfriclite::NetCDFData::getContainer(
    const std::string& name) const {
  oops::Log::debug() << "NetCDFData::getContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end())
    return it->second;
  else
    throw std::runtime_error("NetCDFData::getContainer()> No "
        "definitions of \"" + name + "\"...");
}

std::map<std::string,
         lfriclite::NetCDFDataContainerBase*>& lfriclite::NetCDFData::getContainers() {
  oops::Log::debug() << "NetCDFData::getContainers()" << std::endl;
  return dataContainers_;
}


void lfriclite::NetCDFData::deleteContainer(const std::string& name) {
  oops::Log::debug() << "NetCDFData::deleteContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end()) {
    NetCDFDataContainerBase* netCDFContainer = it->second;
    delete netCDFContainer;
    dataContainers_.erase(name);
  } else {
    throw std::runtime_error("NetCDFData::deleteContainer()> No "
        "definitions of \"" + name + "\"...");
  }
}
