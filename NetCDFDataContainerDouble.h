/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <string>
#include <vector>

#include "NetCDFDataContainerBase.h"

namespace lfriclite {
/// \brief Concrete class for double precision numerical data of a NetCDF file
class NetCDFDataContainerDouble : public NetCDFDataContainerBase {
 public:
  explicit NetCDFDataContainerDouble(const std::string& name);

  NetCDFDataContainerDouble()                      = delete;  //!< Deleted default constructor
  NetCDFDataContainerDouble(const NetCDFDataContainerDouble&) = delete;  //!< Deleted copy construc
  NetCDFDataContainerDouble(NetCDFDataContainerDouble&&)      = delete;  //!< Deleted move construc

  NetCDFDataContainerDouble& operator=(const NetCDFDataContainerDouble&) = delete;  //!< Deleted co
  NetCDFDataContainerDouble& operator=(NetCDFDataContainerDouble&&) = delete;  //!< Deleted move as

  const std::string& getName() const;

  std::vector<double>& getData();
  const double* getDataPointer();
  const double& getDatum(const std::size_t index);

  void resetData();

 private:
  std::vector<double> dataVector_;
};
}  // namespace lfriclite
