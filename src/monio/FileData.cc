/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#include "FileData.h"

monio::FileData::FileData() :
  data_(),
  metadata_() {}

void monio::FileData::clearData() {
  data_.clear();
}

monio::Data& monio::FileData::getData() {
  return data_;
}

const monio::Data& monio::FileData::getData() const {
  return data_;
}

monio::Metadata& monio::FileData::getMetadata() {
  return metadata_;
}

const monio::Metadata& monio::FileData::getMetadata() const {
  return metadata_;
}

std::vector<size_t>& monio::FileData::getLfricAtlasMap() {
  return lfricAtlasMap_;
}

const std::vector<size_t>& monio::FileData::getLfricAtlasMap() const {
  return lfricAtlasMap_;
}

const std::vector<util::DateTime>& monio::FileData::getDateTimes() const {
  return dateTimes_;
}

void monio::FileData::setLfricAtlasMap(std::vector<size_t> lfricAtlasMap) {
  lfricAtlasMap_ = lfricAtlasMap;
}

void monio::FileData::setDateTimes(std::vector<util::DateTime> dateTimes) {
  dateTimes_ = dateTimes;
}
