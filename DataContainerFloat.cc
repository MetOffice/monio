/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/DataContainerFloat.h"

#include <stdexcept>

#include "Constants.h"

monio::DataContainerFloat::DataContainerFloat(const std::string& name) :
  DataContainerBase(name, monio::constants::eFloat) {}

const std::string& monio::DataContainerFloat::getName() const {
  return name_;
}

std::vector<float>& monio::DataContainerFloat::getData() {
  return dataVector_;
}

const float* monio::DataContainerFloat::getDataPointer() {
  return dataVector_.data();
}

const float& monio::DataContainerFloat::getDatum(const std::size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("DataContainerFloat::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void monio::DataContainerFloat::resetData() {
  size_t size = dataVector_.size();
  dataVector_.clear();
  dataVector_.resize(size);
}
