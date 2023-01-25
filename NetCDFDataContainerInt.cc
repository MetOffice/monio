/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFDataContainerInt.h"

#include <stdexcept>

#include "NetCDFConstants.h"

lfriclite::NetCDFDataContainerInt::NetCDFDataContainerInt(const std::string& name) :
  NetCDFDataContainerBase(name, lfriclite::ncconsts::eInt) {}

const std::string& lfriclite::NetCDFDataContainerInt::getName() const {
  return name_;
}

std::vector<int>& lfriclite::NetCDFDataContainerInt::getData() {
  return dataVector_;
}

const int* lfriclite::NetCDFDataContainerInt::getDataPointer() {
  return dataVector_.data();
}

const int& lfriclite::NetCDFDataContainerInt::getDatum(const std::size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("NetCDFDataContainerInt::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void lfriclite::NetCDFDataContainerInt::resetData() {
  size_t size = dataVector_.size();
  dataVector_.clear();
  dataVector_.resize(size);
}
