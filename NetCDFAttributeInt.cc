/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFAttributeInt.h"

#include "NetCDFConstants.h"

lfriclite::NetCDFAttributeInt::NetCDFAttributeInt(const std::string& name, const int value) :
  NetCDFAttributeBase(name, lfriclite::ncconsts::eInt), value_(value) {}

const std::string& lfriclite::NetCDFAttributeInt::getName() const {
  return name_;
}

const int lfriclite::NetCDFAttributeInt::getValue() const {
  return value_;
}
