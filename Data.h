/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>

#include "DataContainerBase.h"

namespace monio {
/// \brief Holds data read from or to be written to a NetCDF file
class Data {
 public:
  Data();
  ~Data();

  Data(const Data&)            = delete;  //!< Deleted copy constructor
  Data(Data&&)                 = delete;  //!< Deleted move constructor

  friend bool operator==(const Data& lhs,
                         const Data& rhs);

  Data& operator=(const Data&) = delete;  //!< Deleted copy assignment
  Data& operator=(Data&&)      = delete;  //!< Deleted move assignment

  void addContainer(DataContainerBase* container);
  DataContainerBase* getContainer(const std::string& name) const;

  std::map<std::string, DataContainerBase*>& getContainers();
  const std::map<std::string, DataContainerBase*>& getContainers() const;

  void deleteContainer(const std::string& name);

 private:
  std::map<std::string, DataContainerBase*> dataContainers_;
};

bool operator==(const Data& lhs,
                const Data& rhs);
}  // namespace monio
