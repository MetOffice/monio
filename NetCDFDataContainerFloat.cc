/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFDataContainerFloat.h"

#include <stdexcept>

#include "NetCDFConstants.h"

lfriclite::NetCDFDataContainerFloat::NetCDFDataContainerFloat(const std::string& name) :
  NetCDFDataContainerBase(name, lfriclite::ncconsts::eFloat) {}

const std::string& lfriclite::NetCDFDataContainerFloat::getName() const {
  return name_;
}

std::vector<float>& lfriclite::NetCDFDataContainerFloat::getData() {
  return dataVector_;
}

const float* lfriclite::NetCDFDataContainerFloat::getDataPointer() {
  return dataVector_.data();
}

const float& lfriclite::NetCDFDataContainerFloat::getDatum(const std::size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("NetCDFDataContainerFloat::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void lfriclite::NetCDFDataContainerFloat::resetData() {
  size_t size = dataVector_.size();
  dataVector_.clear();
  dataVector_.resize(size);
}
