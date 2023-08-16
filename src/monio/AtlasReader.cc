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
                                    const atlas::idx_t mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasReader::AtlasReader()" << std::endl;
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer,
                                      const std::vector<size_t>& lfricToAtlasMap,
                                      const bool copyFirstLevel) {
  oops::Log::debug() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case consts::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData(), lfricToAtlasMap, copyFirstLevel);
      break;
    }
    case consts::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData(), lfricToAtlasMap, copyFirstLevel);
      break;
    }
    case consts::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData(), lfricToAtlasMap, copyFirstLevel);
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
                                       const bool copyFirstLevel) {
  oops::Log::debug() << "AtlasReader::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  atlas::idx_t numLevels = field.levels();
  if (copyFirstLevel == true) {
    numLevels -= 1;
  }
  for (atlas::idx_t j = 0; j < numLevels; ++j) {
    for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
      int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      if (std::size_t(index) <= dataVec.size()) {
        fieldView(i, j) = dataVec[index];
      } else {
        utils::throwException("Calculated index exceeds size of data for field \""
                                 + field.name() + "\".");
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

  std::vector<atlas::idx_t> dimVec = field.shape();
  if (field.metadata().get<bool>("global") == false) {
    dimVec[consts::eHorizontal] = utilsatlas::getSizeOwned(field);
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  atlas::idx_t numLevels = field.levels();
  for (atlas::idx_t i = 0; i < dimVec[consts::eHorizontal]; ++i) {
    for (atlas::idx_t j = 0; j < numLevels; ++j) {
      int index = i + (j * dimVec[consts::eHorizontal]);
      if (std::size_t(index) <= dataVec.size()) {
        fieldView(i, j) = dataVec[index];
      } else {
        utils::throwException("Calculated index exceeds size of data.");
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
