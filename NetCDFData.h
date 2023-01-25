/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>

#include "NetCDFDataContainerBase.h"

namespace lfriclite {
/// \brief Holds data read from or to be written to a NetCDF file
class NetCDFData {
 public:
  NetCDFData();
  ~NetCDFData();

  NetCDFData(const NetCDFData&) = delete;  //!< Deleted copy constructor
  NetCDFData(NetCDFData&&)      = delete;  //!< Deleted move constructor

  friend bool operator==(const NetCDFData& lhs,
                         const NetCDFData& rhs);

  NetCDFData& operator=(const NetCDFData&) = delete;  //!< Deleted copy assign
  NetCDFData& operator=(NetCDFData&&) = delete;       //!< Deleted move assign

  void addContainer(NetCDFDataContainerBase* container);
  NetCDFDataContainerBase* getContainer(const std::string& name) const;

  std::map<std::string, NetCDFDataContainerBase*>& getContainers();

  void deleteContainer(const std::string& name);

 private:
  std::map<std::string, NetCDFDataContainerBase*> dataContainers_;
};

bool operator==(const NetCDFData& lhs,
                const NetCDFData& rhs);
}  // namespace lfriclite
