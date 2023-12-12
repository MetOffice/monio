/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace monio {
/// \brief Contains general helper functions
namespace utils {
  std::vector<std::string> strToWords(const std::string inputStr,
                                      const char separatorChar);

  std::string strNoWhiteSpace(std::string input);
  std::string strTolower(std::string input);

  bool strToBool(std::string input);
  bool fileExists(std::string path);

  std::string exec(const std::string& cmd);

  template<typename T1, typename T2>
  std::vector<T1> extractKeys(std::map<T1, T2> const& inputMap);

  template<typename T>
  int findPosInVector(std::vector<T> vector, T searchTerm);

  template<typename T>
  bool findInVector(std::vector<T> vector, T searchTerm);

  [[noreturn]] void throwException(const std::string message);
}  // namespace utils
}  // namespace monio
