/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace monio {
namespace utils {
  std::vector<std::string> strToWords(const std::string inputStr,
                                      const char separatorChar);

  std::string strNoWhiteSpace(std::string input);
  std::string strTolower(std::string input);

  bool strToBool(std::string input);

  template<typename T1, typename T2>
  std::vector<T1> extractKeys(std::map<T1, T2> const& inputMap);

  template<typename T>
  int findPosInVector(std::vector<T> vector, T searchTerm);

  template<typename T>
  bool findInVector(std::vector<T> vector, T searchTerm);

  [[noreturn]] void throwException(const std::string message);
}  // namespace utils
}  // namespace monio
