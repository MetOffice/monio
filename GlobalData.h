/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include "Data.h"
#include "Metadata.h"

namespace monio {
class GlobalData {
 public:
  static GlobalData* get();

  const monio::Metadata& getMetadata() const;
  const monio::Data& getData() const;

  monio::Metadata& getMetadata();
  monio::Data& getData();

  void setMetadata(monio::Metadata& metadata);
  void setData(monio::Data& data);

 private:
  GlobalData();
  static GlobalData* this_;

  monio::Metadata metadata_;
  monio::Data data_;
};
}  // namespace monio
