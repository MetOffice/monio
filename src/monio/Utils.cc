/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "Utils.h"

#include <stdio.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <memory>

#include "AttributeBase.h"
#include "DataContainerBase.h"
#include "Variable.h"

#include "eckit/mpi/Comm.h"

#include "oops/util/Logger.h"

namespace monio {
namespace utils {
std::vector<std::string> strToWords(const std::string inputStr,
                                    const char separatorChar) {
  std::stringstream stringStream(inputStr);
  std::string word = "";
  std::vector<std::string> wordList;
  while (std::getline(stringStream, word, separatorChar)) {
    wordList.push_back(word);
  }
  return wordList;
}

std::string strNoWhiteSpace(std::string input) {
  input.erase(std::remove_if(input.begin(), input.end(), [](unsigned char x) {
    return std::isspace(x);
  }), input.end());
  return input;
}

std::string strTolower(std::string input) {
  std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return input;
}

bool strToBool(std::string input) {
  std::string cleanStr = strNoWhiteSpace(strTolower(input));
  if ((cleanStr.size() == 1 && cleanStr == "1") ||
      (cleanStr.size() == 4 && cleanStr == "true")) {
    return true;
  } else if ((cleanStr.size() == 1 && cleanStr == "0") ||
             (cleanStr.size() == 5 && cleanStr == "false")) {
    return false;
  } else {
    throw std::invalid_argument("utils::strToBool> Input value of \"" + input + "\" is not valid.");
  }
}

std::string exec(const std::string& cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::string extCmd = "sh -c '" + cmd + "' 2>&1";
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(extCmd.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

bool fileExists(std::string path) {
  std::ifstream f(path.c_str());
  return f.good();
}

template<typename T1, typename T2>
std::vector<T1> extractKeys(std::map<T1, T2> const& inputMap) {
  std::vector<T1> keyVector;
  for (auto const& elementPair : inputMap) {
    keyVector.push_back(elementPair.first);
  }
  return keyVector;
}

template std::vector<std::string> extractKeys<std::string, int>
                        (std::map<std::string, int> const& inputMap);
template std::vector<std::string> extractKeys<std::string, std::shared_ptr<Variable>>
                        (std::map<std::string, std::shared_ptr<Variable>> const& inputMap);
template std::vector<std::string> extractKeys<std::string, std::shared_ptr<AttributeBase>>
                        (std::map<std::string, std::shared_ptr<AttributeBase>> const& inputMap);
template std::vector<std::string> extractKeys<std::string, std::shared_ptr<DataContainerBase>>
                        (std::map<std::string, std::shared_ptr<DataContainerBase>> const& inputMap);

template<typename T>
int findPosInVector(std::vector<T> vector, T searchTerm) {
  int pos = -1;
  typename std::vector<T>::iterator it = std::find(vector.begin(), vector.end(), searchTerm);
  if (it != vector.end()) {
    return std::distance(vector.begin(), it);
  }
  return pos;
}

template int findPosInVector<std::string>(std::vector<std::string> vector, std::string searchTerm);

template<typename T>
bool findInVector(std::vector<T> vector, T searchTerm) {
  typename std::vector<T>::iterator it;
  it = std::find(vector.begin(), vector.end(), searchTerm);
  if (it != vector.end())
    return true;
  else
    return false;
}

template bool findInVector<std::string>(std::vector<std::string> vector, std::string searchTerm);

void throwException(const std::string message) {
  oops::Log::error() << message << std::endl;
  // Call MPI abort on the WORLD communicator.
  eckit::mpi::comm("world").abort();
  throw std::runtime_error(message);
}
}  // namespace utils
}  // namespace monio
