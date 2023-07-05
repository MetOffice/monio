/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "AttributeString.h"

#include "Constants.h"

monio::AttributeString::AttributeString(const std::string& name,
                                                        const std::string& value) :
    AttributeBase(name, monio::consts::eString), value_(value) {}

const std::string& monio::AttributeString::getName() const {
  return name_;
}

const std::string& monio::AttributeString::getValue() const {
  return value_;
}
