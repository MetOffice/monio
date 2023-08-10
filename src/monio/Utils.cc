/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "Utils.h"

#include <algorithm>
#include <memory>

#include "AttributeBase.h"
#include "DataContainerBase.h"
#include "Variable.h"

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
    throw std::invalid_argument("ERROR> Input value of \"" + input + "\" is not valid.");
  }
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
  typename std::vector<T>::iterator it;
  it = std::find(vector.begin(), vector.end(), searchTerm);
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
  oops::Log::debug() << message << std::endl;
  throw std::runtime_error(message);
}
}  // namespace utils
}  // namespace monio
