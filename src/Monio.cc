/*
 * (C) Crown Copyright 2022-2023 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "Monio.h"

#include <memory>
#include <vector>

#include "atlas/parallel/mpi/mpi.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "Constants.h"

namespace {
std::vector<std::string> stringToWords(const std::string& inputStr,
                                       const char separatorChar) {
    std::stringstream stringStream(inputStr);
    std::string word = "";
    std::vector<std::string> wordList;
    while (std::getline(stringStream, word, separatorChar)) {
        wordList.push_back(word);
    }
    return wordList;
}

std::string convertToAtlasDateTimeStr(std::string lfricDateTimeStr) {
  std::vector<std::string> dateTimeSplit = stringToWords(lfricDateTimeStr, ' ');

  return dateTimeSplit[0] + "T" + dateTimeSplit[1] + "Z";
}
}  // anonymous namespace

monio::Monio* monio::Monio::this_ = nullptr;

monio::Monio::~Monio() {
  std::cout << "Monio::~Monio()" << std::endl;
  delete this_;
}

void monio::Monio::readFile(const atlas::CubedSphereGrid& grid,
                                const std::string& filePath,
                                const util::DateTime& date) {
  oops::Log::debug() << "Monio::readFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData& fileData = createFileData(grid.name(), filePath, date);
    // Storage of read data isn't hugely important at this stage. However, keying and storing
    // against the grid name allows for data of different resolutions to be read and available
    // at the point of writing.

    reader_.openFile(fileData);
    reader_.readMetadata(fileData);

    std::vector<std::string> variablesToRead =
        fileData.getMetadata().findVariableNames(monio::constants::kLfricMeshTerm);

    reader_.readSingleData(fileData, variablesToRead);
    createLfricAtlasMap(fileData, grid);

    reader_.readSingleDatum(fileData, monio::constants::kTimeVarName);
    createDateTimes(fileData,
                    monio::constants::kTimeVarName,
                    monio::constants::kTimeOriginName);
  }
}

void monio::Monio::readVarAndPopulateField(const std::string& gridName,
                                           const std::string& varName,
                                           const util::DateTime& date,
                                           const atlas::idx_t& levels,
                                           atlas::Field& globalField) {
  oops::Log::debug() << "Monio::readVarAndPopulateField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    FileData fileData = getFileData(gridName);
    reader_.readFieldDatum(fileData, varName, date, monio::constants::kTimeDimName);
    globalField.set_levels(levels);
    atlasReader_.populateFieldWithDataContainer(globalField,
                                                fileData.getData().getContainer(varName),
                                                fileData.getLfricAtlasMap());
  }
}

void monio::Monio::writeIncrementsFile(const std::string& gridName,
                                   const atlas::FieldSet fieldSet,
                                   const std::vector<std::string>& varNames,
                                   const std::map<std::string, constants::IncrementMetadata>&
                                                                                     incMetadataMap,
                                   const std::string& filePath) {
  oops::Log::debug() << "Monio::writeIncrementsFile()" << std::endl;
  atlas::FieldSet globalFieldSet = atlasProcessor_.getGlobalFieldSet(fieldSet);
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (filePath.length() != 0) {
      FileData fileData = getFileData(gridName);
      atlasWriter_.writeIncrementsToFile(globalFieldSet, varNames,
                                         incMetadataMap, fileData, filePath);
    } else {
      oops::Log::info() << "Monio::writeIncrementsFile() No outputFilePath supplied. "
                           "NetCDF writing will not take place." << std::endl;
    }
  }
}

monio::Monio& monio::Monio::get() {
  oops::Log::debug() << "Monio::get()" << std::endl;
  if (this_ == nullptr) {
    this_ = new Monio(atlas::mpi::comm(), constants::kMPIRankOwner);
  }
  return *this_;
}

void monio::Monio::createLfricAtlasMap(FileData& fileData, const atlas::CubedSphereGrid& grid) {
  oops::Log::debug() << "Monio::createLfricAtlasMap()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getLfricAtlasMap().size() == 0) {
      reader_.readSingleData(fileData, constants::kLfricCoordVarNames);
      std::vector<std::shared_ptr<monio::DataContainerBase>> coordData =
                                reader_.getCoordData(fileData, constants::kLfricCoordVarNames);
      std::vector<atlas::PointLonLat> lfricCoords = atlasProcessor_.getLfricCoords(coordData);
      std::vector<atlas::PointLonLat> atlasCoords = atlasProcessor_.getAtlasCoords(grid);
      fileData.setLfricAtlasMap(atlasProcessor_.createLfricAtlasMap(atlasCoords, lfricCoords));
    }
  }
}

void monio::Monio::createDateTimes(FileData& fileData,
                                 const std::string& timeVarName,
                                 const std::string& timeOriginName) {
  oops::Log::debug() << "Monio::createDateTimes()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (fileData.getDateTimes().size() == 0) {
      std::shared_ptr<Variable> timeVar = fileData.getMetadata().getVariable(timeVarName);
      std::shared_ptr<DataContainerBase> timeDataBase =
                                             fileData.getData().getContainer(timeVarName);
      if (timeDataBase->getType() != constants::eDouble)
        throw std::runtime_error("Monio::createDateTimes()> "
                                 "Time data not stored as double...");

      std::shared_ptr<DataContainerDouble> timeData =
                  std::static_pointer_cast<DataContainerDouble>(timeDataBase);

      std::string timeOrigin = timeVar->getStrAttr(timeOriginName);
      std::string timeAtlasOrigin = convertToAtlasDateTimeStr(timeOrigin);

      util::DateTime originDateTime(timeAtlasOrigin);

      oops::Log::debug() << "timeVar->getSize()> " << timeVar->getTotalSize() << std::endl;
      std::vector<util::DateTime> dateTimes(timeVar->getTotalSize());
      for (std::size_t index = 0; index < timeVar->getTotalSize(); ++index) {
        util::Duration duration(static_cast<uint64_t>(std::round(timeData->getDatum(index))));
        util::DateTime dateTime = originDateTime + duration;
        dateTimes[index] = dateTime;
        oops::Log::debug() << "index> " << index << ", data> " << timeData->getDatum(index) <<
                             ", dateTime> " << dateTime << std::endl;
      }
      fileData.setDateTimes(std::move(dateTimes));
    }
  }
}

monio::FileData& monio::Monio::createFileData(const std::string& gridName,
                                                      const std::string& filePath,
                                                      const util::DateTime& date) {
  oops::Log::debug() << "Monio::createFileData()" << std::endl;
  auto it = filesData_.find(gridName);

  if (it != filesData_.end()) {
    filesData_.erase(gridName);
  }
  // Overwrite existing data
  filesData_.insert({gridName, FileData(filePath, date)});
  return filesData_.at(gridName);
}

monio::FileData monio::Monio::getFileData(const std::string& gridName) {
  oops::Log::debug() << "Monio::getFileData()" << std::endl;
  auto it = filesData_.find(gridName);
  if (it != filesData_.end()) {
    return FileData(it->second);
  }
  throw std::runtime_error("Monio::getFileData()> FileData with grid name \"" +
                           gridName + "\" not found...");
}

monio::Monio::Monio(const eckit::mpi::Comm& mpiCommunicator,
                    const atlas::idx_t mpiRankOwner) :
      mpiCommunicator_(mpiCommunicator),
      mpiRankOwner_(mpiRankOwner),
      reader_(mpiCommunicator, mpiRankOwner_),
      atlasProcessor_(mpiCommunicator, mpiRankOwner_),
      atlasReader_(mpiCommunicator, mpiRankOwner_),
      atlasWriter_(mpiCommunicator, mpiRankOwner_)   {
  oops::Log::debug() << "Monio::Monio()" << std::endl;
}
