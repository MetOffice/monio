/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "AttributeString.h"

#include "Constants.h"

monio::AttributeString::AttributeString(const std::string& name,
                                                        const std::string& value) :
    AttributeBase(name, consts::eString), value_(value) {}

const std::string& monio::AttributeString::getName() const {
  return name_;
}

const std::string& monio::AttributeString::getValue() const {
  return value_;
}
