/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
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
