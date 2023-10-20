/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "DataContainerBase.h"

namespace monio {
/// \brief Holds data read from or to be written to a NetCDF file stored as data containers.
class Data {
 public:
  Data();

  /// \brief Custom equality operator is a friend for access to private class members.
  friend bool operator==(const Data& lhs, const Data& rhs);

  void addContainer(std::shared_ptr<DataContainerBase> container);
  void deleteContainer(const std::string& name);
  void removeAllButTheseContainers(const std::vector<std::string>& names);

  bool isContainerPresent(const std::string& name) const;

  std::shared_ptr<monio::DataContainerBase> getContainer(const std::string& name) const;

  std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& getContainers();
  const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& getContainers() const;

  std::vector<std::string> getDataContainerNames() const;

  /// \brief Clears data for memory-efficiency. Written data can be dropped before writing
  ///        subsequent variables.
  void clear();

 private:
  std::map<std::string, std::shared_ptr<DataContainerBase>> dataContainers_;
};

/// \brief Equality operator declaration for visibility outside of class.
bool operator==(const Data& lhs,
                const Data& rhs);
}  // namespace monio
