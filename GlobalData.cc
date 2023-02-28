/*
 * (C) Crown Copyright 2022-2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "lfriclitejedi/IO/GlobalData.h"

#include "oops/util/Logger.h"

monio::GlobalData* monio::GlobalData::this_ = nullptr;

monio::GlobalData* monio::GlobalData::get() {
  oops::Log::trace() << "GlobalData::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new GlobalData();
  }
  return this_;
}

monio::GlobalData::GlobalData() {
  oops::Log::trace() << "GlobalData::GlobalData()" << std::endl;
}

const monio::Metadata& monio::GlobalData::getMetadata() const {
  oops::Log::trace() << "GlobalData::getMetadata()" << std::endl;
  return metadata_;
}

const monio::Data& monio::GlobalData::getData() const {
  oops::Log::trace() << "GlobalData::getData()" << std::endl;
  return data_;
}

monio::Metadata& monio::GlobalData::getMetadata() {
  oops::Log::trace() << "GlobalData::getMetadata()" << std::endl;
  return metadata_;
}
monio::Data& monio::GlobalData::getData() {
  oops::Log::trace() << "GlobalData::getData()" << std::endl;
  return data_;
}

void monio::GlobalData::setMetadata(monio::Metadata& metadata) {
  oops::Log::trace() << "GlobalData::setMetadata()" << std::endl;
  metadata_ = metadata;
}

void monio::GlobalData::setData(monio::Data& data) {
  oops::Log::trace() << "GlobalData::setData()" << std::endl;
  data_ = data;
}
