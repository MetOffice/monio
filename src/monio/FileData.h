/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
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

  /// \brief Clears contents of Data and all except dimensions in Metadata. Used for
  ///        memory-efficiency where all but required data can be dropped.
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
