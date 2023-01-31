/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/Data.h"

#include <stdexcept>
#include <vector>

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"

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

monio::Data::Data() {}

monio::Data::~Data() {
  for (auto it = dataContainers_.begin(); it != dataContainers_.end(); ++it) {
    delete it->second;
  }
}

bool monio::operator==(const monio::Data& lhs, const monio::Data& rhs) {
  if (lhs.dataContainers_.size() == rhs.dataContainers_.size()) {
    for (auto lhsIt = lhs.dataContainers_.begin(), rhsIt = rhs.dataContainers_.begin();
         lhsIt != lhs.dataContainers_.end(); ++lhsIt , ++rhsIt)
    {
      DataContainerBase* lhsDataContainer = lhsIt->second;
      DataContainerBase* rhsDataContainer = rhsIt->second;

      int lhsDataType = lhsDataContainer->getType();
      int rhsDataType = rhsDataContainer->getType();

      std::string lhsName = lhsDataContainer->getName();
      std::string rhsName = rhsDataContainer->getName();

      if (lhsDataType == rhsDataType && lhsName == rhsName) {
        switch (lhsDataType) {
          case monio::constants::eDataTypes::eDouble: {
            DataContainerDouble* lhsDataContainerDouble =
              static_cast<DataContainerDouble*>(lhsDataContainer);
            DataContainerDouble* rhsDataContainerDouble =
              static_cast<DataContainerDouble*>(rhsDataContainer);
            if (compareData(lhsDataContainerDouble->getData(),
                          rhsDataContainerDouble->getData()) == false)
              return false;
            break;
          }
          case monio::constants::eDataTypes::eFloat: {
            DataContainerFloat* lhsDataContainerFloat =
              static_cast<DataContainerFloat*>(lhsDataContainer);
            DataContainerFloat* rhsDataContainerFloat =
              static_cast<DataContainerFloat*>(rhsDataContainer);
            if (compareData(lhsDataContainerFloat->getData(),
                          rhsDataContainerFloat->getData()) == false)
              return false;
            break;
          }
          case monio::constants::eDataTypes::eInt: {
            DataContainerInt* lhsDataContainerInt =
              static_cast<DataContainerInt*>(lhsDataContainer);
            DataContainerInt* rhsDataContainerInt =
              static_cast<DataContainerInt*>(rhsDataContainer);
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

void monio::Data::addContainer(DataContainerBase* container) {
  oops::Log::debug() << "Data::addContainer()" << std::endl;
  const std::string& name = container->getName();
  auto it = dataContainers_.find(name);
  if (it == dataContainers_.end())
    dataContainers_.insert({name, container});
  else
    throw std::runtime_error("Data::addContainer()> multiple "
        "definitions of \"" + name + "\"...");
}

monio::DataContainerBase* monio::Data::getContainer(const std::string& name) const {
  oops::Log::debug() << "Data::getContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end())
    return it->second;
  else
    throw std::runtime_error("Data::getContainer()> No "
        "definitions of \"" + name + "\"...");
}

std::map<std::string, monio::DataContainerBase*>& monio::Data::getContainers() {
  oops::Log::debug() << "Data::getContainers()" << std::endl;
  return dataContainers_;
}

const std::map<std::string, monio::DataContainerBase*>& monio::Data::getContainers() const {
  oops::Log::debug() << "Data::getContainers()" << std::endl;
  return dataContainers_;
}



void monio::Data::deleteContainer(const std::string& name) {
  oops::Log::debug() << "Data::deleteContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end()) {
    DataContainerBase* netCDFContainer = it->second;
    delete netCDFContainer;
    dataContainers_.erase(name);
  } else {
    throw std::runtime_error("Data::deleteContainer()> No "
        "definitions of \"" + name + "\"...");
  }
}
