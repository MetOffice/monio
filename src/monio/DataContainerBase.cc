/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "DataContainerBase.h"

monio::DataContainerBase::DataContainerBase(
    const std::string& name,
    const int type) :
  name_(name), type_(type) {}

const int monio::DataContainerBase::getType() const {
  return type_;
}
