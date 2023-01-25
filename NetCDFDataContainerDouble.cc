/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFDataContainerDouble.h"

#include <stdexcept>

#include "NetCDFConstants.h"

lfriclite::NetCDFDataContainerDouble::NetCDFDataContainerDouble(const std::string& name) :
  NetCDFDataContainerBase(name, lfriclite::ncconsts::eDouble) {}

const std::string& lfriclite::NetCDFDataContainerDouble::getName() const {
  return name_;
}

std::vector<double>& lfriclite::NetCDFDataContainerDouble::getData() {
  return dataVector_;
}

const double* lfriclite::NetCDFDataContainerDouble::getDataPointer() {
  return dataVector_.data();
}

const double& lfriclite::NetCDFDataContainerDouble::getDatum(const std::size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("NetCDFDataContainerDouble::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void lfriclite::NetCDFDataContainerDouble::resetData() {
  size_t size = dataVector_.size();
  dataVector_.clear();
  dataVector_.resize(size);
}
