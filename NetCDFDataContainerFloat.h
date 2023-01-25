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
/// \brief Concrete class for single precision numerical data of a NetCDF file
class NetCDFDataContainerFloat : public NetCDFDataContainerBase {
 public:
  explicit NetCDFDataContainerFloat(const std::string& name);

  NetCDFDataContainerFloat()                      = delete;  //!< Deleted default constructor
  NetCDFDataContainerFloat(const NetCDFDataContainerFloat&) = delete;  //!< Deleted copy constructo
  NetCDFDataContainerFloat(NetCDFDataContainerFloat&&)      = delete;  //!< Deleted move constructo

  NetCDFDataContainerFloat& operator=(const NetCDFDataContainerFloat&) = delete;  //!< Deleted copy
  NetCDFDataContainerFloat& operator=(NetCDFDataContainerFloat&&) = delete;  //!< Deleted move assi

  const std::string& getName() const;

  std::vector<float>& getData();
  const float* getDataPointer();
  const float& getDatum(const std::size_t index);

  void resetData();

 private:
  std::vector<float> dataVector_;
};
}  // namespace lfriclite
