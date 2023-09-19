/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "AtlasReader.h"

#include "oops/util/Logger.h"

#include "Utils.h"
#include "UtilsAtlas.h"

monio::AtlasReader::AtlasReader(const eckit::mpi::Comm& mpiCommunicator,
                                    const int mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasReader::AtlasReader()" << std::endl;
}

void monio::AtlasReader::populateFieldWithFileData(atlas::Field& field,
                                             const FileData& fileData,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& readName) {
  oops::Log::debug() << "AtlasReader::populateFieldWithFileData()" << std::endl;
  atlas::Field formattedField = getReadField(field, fieldMetadata.noFirstLevel);
  populateFieldWithDataContainer(formattedField,
                                 fileData.getData().getContainer(readName),
                                 fileData.getLfricAtlasMap(),
                                 fieldMetadata.noFirstLevel);
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer,
                                      const std::vector<size_t>& lfricToAtlasMap,
                                      const bool noFirstLevel) {
  oops::Log::debug() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case consts::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData(), lfricToAtlasMap, noFirstLevel);
      break;
    }
    case consts::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData(), lfricToAtlasMap, noFirstLevel);
      break;
    }
    case consts::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData(), lfricToAtlasMap, noFirstLevel);
      break;
    }
    default:
      utils::throwException("AtlasReader::populateFieldWithDataContainer()> "
                               "Data type not coded for...");
    }
  }
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer) {
  oops::Log::debug() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case consts::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData());
      break;
    }
    case consts::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData());
      break;
    }
    case consts::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData());
      break;
    }
    default:
      utils::throwException("AtlasReader::populateFieldWithDataContainer()> "
                               "Data type not coded for...");
    }
  }
}

template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                 const std::vector<T>& dataVec,
                                 const std::vector<size_t>& lfricToAtlasMap,
                                 const bool noFirstLevel) {
  oops::Log::debug() << "AtlasReader::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  // Erroneous case for field with noFirstLevel == true should have been adjusted by getReadField()
  // to have 70 and not 71 levels.
  if (noFirstLevel == true && field.levels() == consts::kVerticalFullSize) {
    utils::throwException("AtlasReader::populateField()> Field levels misconfiguration...");
  // Only valid case for field with noFirstLevel == true. Field is adjusted to have 70 levels but
  // read data still has enough to fill 71.
  } else if (noFirstLevel == true && field.levels() == consts::kVerticalHalfSize) {
    for (int j = 1; j < consts::kVerticalFullSize; ++j) {
      for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
        int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
        // Bounds checking
        if (std::size_t(index) <= dataVec.size()) {
          fieldView(i, j - 1) = dataVec[index];
        } else {
          utils::throwException("AtlasReader::populateField()> Calculated index exceeds size of "
                                "data for field \"" + field.name() + "\".");
        }
      }
    }
  // Valid case for fields noFirstLevel == false. Field is filled with all available data.
  } else {
    for (int j = 0; j < field.levels(); ++j) {
      for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
        int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
        // Bounds checking
        if (std::size_t(index) <= dataVec.size()) {
          fieldView(i, j) = dataVec[index];
        } else {
          utils::throwException("AtlasReader::populateField()> Calculated index exceeds size of "
                                "data for field \"" + field.name() + "\".");
        }
      }
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec,
                                                        const std::vector<size_t>& lfricToAtlasMap,
                                                        const bool copyFirstLevel);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec,
                                                       const std::vector<size_t>& lfricToAtlasMap,
                                                        const bool copyFirstLevel);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec,
                                                     const std::vector<size_t>& lfricToAtlasMap,
                                                     const bool copyFirstLevel);

template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                       const std::vector<T>& dataVec) {
  oops::Log::debug() << "AtlasReader::populateField()" << std::endl;

  std::vector<int> dimVec = field.shape();
  if (field.metadata().get<bool>("global") == false) {
    dimVec[consts::eHorizontal] = utilsatlas::getHorizontalSize(field);
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  int numLevels = field.levels();
  for (int i = 0; i < dimVec[consts::eHorizontal]; ++i) {
    for (int j = 0; j < numLevels; ++j) {
      int index = i + (j * dimVec[consts::eHorizontal]);
      if (std::size_t(index) <= dataVec.size()) {
        fieldView(i, j) = dataVec[index];
      } else {
        utils::throwException("AtlasReader::populateField()> "
                              "Calculated index exceeds size of data.");
      }
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec);

atlas::Field monio::AtlasReader::getReadField(atlas::Field& field,
                                              const bool noFirstLevel) {
  if (noFirstLevel == true && field.levels() == consts::kVerticalHalfSize) {
    utils::throwException("AtlasReader::getReadField()> Field levels misconfiguration...");
  }
  if (noFirstLevel == true) {
    atlas::array::DataType atlasType = field.datatype();
    if (atlasType != atlasType.KIND_REAL64 &&
        atlasType != atlasType.KIND_REAL32 &&
        atlasType != atlasType.KIND_INT32) {
        utils::throwException("AtlasReader::getReadField())> Data type not coded for...");
    }
    atlas::util::Config atlasOptions = atlas::option::name(field.name()) |
                                       atlas::option::levels(consts::kVerticalHalfSize) |
                                       atlas::option::datatype(atlasType) |
                                       atlas::option::global(0);
    const auto& functionSpace = field.functionspace();
    return functionSpace.createField(atlasOptions);
  } else {
    return field;
  }
}
