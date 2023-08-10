/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
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
