/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "DataContainerInt.h"

#include <stdexcept>

#include "Constants.h"

monio::DataContainerInt::DataContainerInt(const std::string& name) :
  DataContainerBase(name, monio::consts::eInt) {}

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

const int& monio::DataContainerInt::getDatum(const size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("DataContainerInt::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void monio::DataContainerInt::setData(const std::vector<int> dataVector) {
  dataVector_ = dataVector;
}

void monio::DataContainerInt::setDatum(const size_t index, const int datum) {
  dataVector_[index] = datum;
}

void monio::DataContainerInt::setDatum(const int datum) {
  dataVector_.push_back(datum);
}

void monio::DataContainerInt::setSize(const int size) {
  dataVector_.resize(size);
}

void monio::DataContainerInt::clear() {
  dataVector_.clear();
}
