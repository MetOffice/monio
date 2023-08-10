/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "DataContainerFloat.h"

#include <stdexcept>

#include "Constants.h"

monio::DataContainerFloat::DataContainerFloat(const std::string& name) :
  DataContainerBase(name, consts::eFloat) {}

const std::string& monio::DataContainerFloat::getName() const {
  return name_;
}

std::vector<float>& monio::DataContainerFloat::getData() {
  return dataVector_;
}

const std::vector<float>& monio::DataContainerFloat::getData() const {
  return dataVector_;
}

const float* monio::DataContainerFloat::getDataPointer() {
  return dataVector_.data();
}

const float& monio::DataContainerFloat::getDatum(const size_t index) {
  if (index > dataVector_.size())
    throw std::runtime_error("DataContainerFloat::getDatum()> "
        "Passed index exceeds vector size...");

  return dataVector_[index];
}

void monio::DataContainerFloat::setData(const std::vector<float> dataVector) {
  dataVector_ = dataVector;
}

void monio::DataContainerFloat::setDatum(const size_t index, const float datum) {
  dataVector_[index] = datum;
}

void monio::DataContainerFloat::setDatum(const float datum) {
  dataVector_.push_back(datum);
}

void monio::DataContainerFloat::setSize(const int size) {
  dataVector_.resize(size);
}

void monio::DataContainerFloat::clear() {
  dataVector_.clear();
}
