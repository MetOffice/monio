/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/NetCDFAttributeString.h"

#include "NetCDFConstants.h"

lfriclite::NetCDFAttributeString::NetCDFAttributeString(const std::string& name,
                                                        const std::string& value) :
    NetCDFAttributeBase(name, lfriclite::ncconsts::eString), value_(value) {}

const std::string& lfriclite::NetCDFAttributeString::getName() const {
  return name_;
}

const std::string& lfriclite::NetCDFAttributeString::getValue() const {
  return value_;
}
