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

monio::AtlasReader::AtlasReader(const eckit::mpi::Comm& mpiCommunicator,
                                    const atlas::idx_t mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner),
    atlasProcessor_(mpiCommunicator, mpiRankOwner) {
  oops::Log::debug() << "AtlasReader::AtlasReader()" << std::endl;
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer,
                                      const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
    case constants::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData(), lfricToAtlasMap);
      break;
    }
    case constants::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData(), lfricToAtlasMap);
      break;
    }
    case constants::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData(), lfricToAtlasMap);
      break;
    }
    default:
      throw std::runtime_error("AtlasReader::populateFieldWithDataContainer()> "
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
    case constants::eDataTypes::eDouble: {
      const std::shared_ptr<DataContainerDouble> dataContainerDouble =
          std::static_pointer_cast<DataContainerDouble>(dataContainer);
          populateField(field, dataContainerDouble->getData());
      break;
    }
    case constants::eDataTypes::eFloat: {
      const std::shared_ptr<DataContainerFloat> dataContainerFloat =
          std::static_pointer_cast<DataContainerFloat>(dataContainer);
          populateField(field, dataContainerFloat->getData());
      break;
    }
    case constants::eDataTypes::eInt: {
      const std::shared_ptr<DataContainerInt> dataContainerInt =
          std::static_pointer_cast<DataContainerInt>(dataContainer);
          populateField(field, dataContainerInt->getData());
      break;
    }
    default:
      throw std::runtime_error("AtlasReader::populateFieldWithDataContainer()> "
                               "Data type not coded for...");
    }
  }
}

void monio::AtlasReader::populateFieldSetWithData(atlas::FieldSet& fieldSet,
                                                const Data& data) {
  oops::Log::debug() << "AtlasReader::populateFieldSetWithData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    for (auto& field : fieldSet) {
      populateFieldWithDataContainer(field, data.getContainer(field.name()));
    }
  }
}

void monio::AtlasReader::populateFieldSetWithData(atlas::FieldSet& fieldSet,
                                                const Data& data,
                                                const std::vector<std::string>& fieldNames) {
  oops::Log::debug() << "AtlasReader::populateFieldSetWithData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    auto fieldNameIt = fieldNames.begin();
    for (auto& field : fieldSet) {
      populateFieldWithDataContainer(field, data.getContainer(*fieldNameIt));
      ++fieldNameIt;
    }
  }
}


template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                       const std::vector<T>& dataVec,
                                       const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasReader::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  atlas::idx_t numLevels = field.levels();
  for (atlas::idx_t j = 0; j < numLevels; ++j) {
    for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
      int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec,
                                                        const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec,
                                                       const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec,
                                                     const std::vector<size_t>& lfricToAtlasMap);

template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                     const std::vector<T>& dataVec) {
  oops::Log::debug() << "AtlasReader::populateField()" << std::endl;

  std::vector<atlas::idx_t> dimVec = field.shape();
  if (field.metadata().get<bool>("global") == false) {
    dimVec[constants::eHorizontal] = atlasProcessor_.getSizeOwned(field);
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  atlas::idx_t numLevels = field.levels();
  for (atlas::idx_t i = 0; i < dimVec[constants::eHorizontal]; ++i) {
    for (atlas::idx_t j = 0; j < numLevels; ++j) {
      int index = i + (j * dimVec[constants::eHorizontal]);
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec);
