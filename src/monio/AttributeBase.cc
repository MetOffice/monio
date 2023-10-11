/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "AttributeBase.h"

monio::AttributeBase::AttributeBase(
    const std::string& name,
    const int type):
  name_(name), type_(type) {}

const int monio::AttributeBase::getType() const {
  return type_;
}
