/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/DataContainerInt.h"

#include <stdexcept>

#include "Constants.h"

monio::DataContainerInt::DataContainerInt(const std::string& name) :
  DataContainerBase(name, monio::constants::eInt) {}

const std::string& monio::DataContainerInt::getName() const {
  return name_;
}

std::vector<int>& monio::DataContainerInt::getData() {
  return dataVector_;
}

const std::vector<int>& monio::DataContainerInt::getData() const {
  return dataVector_;
}

const int* monio::DataContainerInt::getDataPointer() {
  return dataVector_.data();
}

const int& monio::DataContainerInt::getDatum(const std::size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("DataContainerInt::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void monio::DataContainerInt::resetData() {
  size_t size = dataVector_.size();
  dataVector_.clear();
  dataVector_.resize(size);
}
