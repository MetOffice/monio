/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "DataContainerBase.h"

namespace monio {
/// \brief Holds data read from or to be written to a NetCDF file
class Data {
 public:
  Data();

  friend bool operator==(const Data& lhs,
                         const Data& rhs);

  void addContainer(std::shared_ptr<DataContainerBase> container);
  std::shared_ptr<monio::DataContainerBase> getContainer(const std::string& name) const;

  std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& getContainers();
  const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>& getContainers() const;

  void deleteContainer(const std::string& name);
  std::vector<std::string> getDataContainerNames();

  void removeAllButTheseContainers(const std::vector<std::string>& varNames);

 private:
  std::map<std::string, std::shared_ptr<DataContainerBase>> dataContainers_;
};

bool operator==(const Data& lhs,
                const Data& rhs);
}  // namespace monio