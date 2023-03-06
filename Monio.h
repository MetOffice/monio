/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include "AtlasProcessor.h"
#include "Reader.h"

namespace monio {
class Monio {
 public:
  static Monio* get();

  const monio::Reader& getReader() const;
  monio::Reader& getReader();

  const monio::AtlasProcessor& getAtlasProcessor() const;

 private:
  Monio();
  static Monio* this_;

  monio::Reader reader_;
  monio::AtlasProcessor atlasProcessor_;
};
}  // namespace monio
