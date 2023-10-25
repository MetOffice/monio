/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "AttributeInt.h"

#include "Constants.h"

monio::AttributeInt::AttributeInt(const std::string& name, const int value) :
  AttributeBase(name, consts::eInt), value_(value) {}

const std::string& monio::AttributeInt::getName() const {
  return name_;
}

const int monio::AttributeInt::getValue() const {
  return value_;
}
