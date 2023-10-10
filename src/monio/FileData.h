/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <string>
#include <vector>

#include "oops/util/DateTime.h"

#include "Data.h"
#include "Metadata.h"

namespace monio {
/// \brief Packages data and metadata associated with a file being read or written to. Used
///        primarily in MONIO for keeping a copy of read metadata that is used for writing to
///        LFRic-format output files.
class FileData {
 public:
  FileData();

  /// \brief Clears only data. Used for memory-efficiency where written data can be dropped before
  ///        writing subsequent variables.
  void clearData();

  Data& getData();
  const Data& getData() const;
  Metadata& getMetadata();
  const Metadata& getMetadata() const;
  std::vector<size_t>& getLfricAtlasMap();
  const std::vector<size_t>& getLfricAtlasMap() const;
  const std::vector<util::DateTime>& getDateTimes() const;

  void setDate(util::DateTime);
  void setLfricAtlasMap(std::vector<size_t>);
  void setDateTimes(std::vector<util::DateTime>);

 private:
  Data data_;
  Metadata metadata_;

  /// \brief Mapping between Atlas and LFRic coordinate/data order, if applicable.
  std::vector<size_t> lfricAtlasMap_;
  /// \brief Date-times from read file, if present.
  std::vector<util::DateTime> dateTimes_;
};
}  // namespace monio
