/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "DataContainerDouble.h"

#include <stdexcept>

#include "Constants.h"
#include "Monio.h"
#include "Utils.h"

monio::DataContainerDouble::DataContainerDouble(const std::string& name) :
  DataContainerBase(name, consts::eDouble) {}

const std::string& monio::DataContainerDouble::getName() const {
  return name_;
}

std::vector<double>& monio::DataContainerDouble::getData() {
  return dataVector_;
}

const std::vector<double>& monio::DataContainerDouble::getData() const {
  return dataVector_;
}

const double* monio::DataContainerDouble::getDataPointer() {
  return dataVector_.data();
}

const double& monio::DataContainerDouble::getDatum(const size_t index) {
  if (index > dataVector_.size()) {
    Monio::get().closeFiles();
    utils::throwException("DataContainerDouble::getDatum()> Passed index exceeds vector size...");
  }

  return dataVector_[index];
}

void monio::DataContainerDouble::setData(const std::vector<double> dataVector) {
  dataVector_ = dataVector;
}

void monio::DataContainerDouble::setDatum(const size_t index, const double datum) {
  dataVector_[index] = datum;
}

void monio::DataContainerDouble::setDatum(const double datum) {
  dataVector_.push_back(datum);
}

void monio::DataContainerDouble::setSize(const int size) {
  dataVector_.resize(size);
}

void monio::DataContainerDouble::clear() {
  dataVector_.clear();
}
