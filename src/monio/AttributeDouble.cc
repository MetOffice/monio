/*
 * (C) Crown Copyright 2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "AttributeDouble.h"

#include "Constants.h"

monio::AttributeDouble::AttributeDouble(const std::string& name, const double value) :
  AttributeBase(name, consts::eDouble), value_(value) {}

const std::string& monio::AttributeDouble::getName() const {
  return name_;
}

const double monio::AttributeDouble::getValue() const {
  return value_;
}
