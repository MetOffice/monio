/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#include "AttributeBase.h"

monio::AttributeBase::AttributeBase(
    const std::string& name,
    const int type):
  name_(name), type_(type) {}

const int monio::AttributeBase::getType() const {
  return type_;
}
