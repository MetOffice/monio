/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

#include "oops/util/DateTime.h"

#include "Data.h"
#include "Metadata.h"

namespace monio {
class FileData {
 public:
  explicit FileData(const std::string filePath);

  Data& getData();
  const Data& getData() const;
  Metadata& getMetadata();
  const Metadata& getMetadata() const;
  const std::string& getFilePath() const;
  std::vector<size_t>& getLfricAtlasMap();
  const std::vector<size_t>& getLfricAtlasMap() const;
  const std::vector<util::DateTime>& getDateTimes() const;

  void setFilePath(std::string);
  void setLfricAtlasMap(std::vector<size_t>);
  void setDateTimes(std::vector<util::DateTime>);

 private:
  Data data_;
  Metadata metadata_;

  std::string filePath_;
  std::vector<size_t> lfricAtlasMap_;
  std::vector<util::DateTime> dateTimes_;
};
}  // namespace monio
