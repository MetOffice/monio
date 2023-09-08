
/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "AtlasWriter.h"

#include "atlas/grid/Iterator.h"
#include "oops/util/Logger.h"

#include "AttributeString.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Metadata.h"
#include "Utils.h"
#include "UtilsAtlas.h"
#include "Writer.h"

monio::AtlasWriter::AtlasWriter(const eckit::mpi::Comm& mpiCommunicator,
                                    const int mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasWriter::AtlasWriter()" << std::endl;
}

void monio::AtlasWriter::populateFileDataWithField(FileData& fileData,
                                             const atlas::Field& field) {
  oops::Log::debug() << "AtlasWriter::populateFileDataWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    Metadata& metadata = fileData.getMetadata();
    Data& data = fileData.getData();
    // Create dimensions
    std::vector<int> dimVec = field.shape();
    if (field.metadata().get<bool>("global") == false) {
      dimVec[0] = utilsatlas::getHorizontalSize(field);
    }
    for (auto& dimSize : dimVec) {
      std::string dimName = metadata.getDimensionName(dimSize);
      if (dimName == consts::kNotFoundError) {
        dimName = "dim" + std::to_string(dimCount_);
        fileData.getMetadata().addDimension(dimName, dimSize);
        dimCount_++;
      }
    }
    // Create metadata
    populateMetadataWithField(metadata, field);
    // Create lon and lat
    std::vector<atlas::PointLonLat> atlasLonLat = utilsatlas::getAtlasCoords(field);
    std::vector<std::shared_ptr<DataContainerBase>> coordContainers =
              utilsatlas::convertLatLonToContainers(atlasLonLat, consts::kCoordVarNames);
    for (const auto& coordContainer : coordContainers) {
      data.addContainer(coordContainer);
    }
    std::shared_ptr<monio::Variable> lonVar = std::make_shared<Variable>(
                  consts::kCoordVarNames[consts::eLongitude], consts::eDouble);
    std::shared_ptr<monio::Variable> latVar = std::make_shared<Variable>(
                  consts::kCoordVarNames[consts::eLatitude], consts::eDouble);
    std::string dimName = metadata.getDimensionName(atlasLonLat.size());
    lonVar->addDimension(dimName, atlasLonLat.size());
    latVar->addDimension(dimName, atlasLonLat.size());
    metadata.addVariable(consts::kCoordVarNames[consts::eLongitude], lonVar);
    metadata.addVariable(consts::kCoordVarNames[consts::eLatitude], latVar);

    populateDataWithField(data, field, dimVec);
    // Global attrs
    std::string producedByName = "Produced_by";
    std::string producedByString = "MONIO: Met Office NetCDF I/O";

    std::shared_ptr<monio::AttributeString> producedByAttr =
            std::make_shared<AttributeString>(producedByName, producedByString);
    fileData.getMetadata().addGlobalAttr(producedByName, producedByAttr);
  }
}

void monio::AtlasWriter::populateFileDataWithField(FileData& fileData,
                                                   atlas::Field& field,
                                             const std::string& writeName,
                                             const bool copyFirstLevel) {
  oops::Log::debug() << "AtlasWriter::populateFileDataWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    Metadata& metadata = fileData.getMetadata();
    Data& data = fileData.getData();
    std::vector<size_t>& lfricAtlasMap = fileData.getLfricAtlasMap();
    // Create dimensions
    metadata.addDimension(std::string(consts::kHorizontalName), lfricAtlasMap.size());
    metadata.addDimension(std::string(consts::kVerticalFullName), consts::kVerticalFullSize);
    metadata.addDimension(std::string(consts::kVerticalHalfName), consts::kVerticalHalfSize);

    // Create metadata
    populateMetadataWithField(metadata, field);

    // Data
    atlas::Field formattedField = utilsatlas::getFormattedField(field, writeName, copyFirstLevel);

    populateDataWithField(data, formattedField, lfricAtlasMap);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field) {
  oops::Log::debug() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  std::string varName = field.name();
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  std::vector<int> dimVec = field.shape();

  // Check if Field is not global
  if (field.metadata().get<bool>("global") == false) {
    dimVec[0] = utilsatlas::getHorizontalSize(field);  // If so, get the 2D size of the Field
  }
  for (auto& dimSize : dimVec) {
    std::string dimName = metadata.getDimensionName(dimSize);
    if (dimName != consts::kNotFoundError) {  // Not used for 1-D fields.
      var->addDimension(dimName, dimSize);
    }
  }
  metadata.addVariable(varName, var);
}

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::string fieldName = field.name();
    atlas::array::DataType atlasType = field.datatype();
    int fieldSize = utilsatlas::getDataSize(field);
    switch (atlasType.kind()) {
      case atlasType.KIND_INT32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerInt>(fieldName);
        }
        std::shared_ptr<DataContainerInt> dataContainerInt =
                          std::static_pointer_cast<DataContainerInt>(dataContainer);
        dataContainerInt->clear();
        dataContainerInt->setSize(fieldSize);
        populateDataVec(dataContainerInt->getData(), field, lfricToAtlasMap);
        break;
      }
      case atlasType.KIND_REAL32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerFloat>(fieldName);
        }
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                          std::static_pointer_cast<DataContainerFloat>(dataContainer);
        dataContainerFloat->clear();
        dataContainerFloat->setSize(fieldSize);
        populateDataVec(dataContainerFloat->getData(), field, lfricToAtlasMap);
        break;
      }
      case atlasType.KIND_REAL64: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerDouble>(fieldName);
        }
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                          std::static_pointer_cast<DataContainerDouble>(dataContainer);
        dataContainerDouble->clear();
        dataContainerDouble->setSize(fieldSize);
        populateDataVec(dataContainerDouble->getData(), field, lfricToAtlasMap);
        break;
      }
      default: {
        utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                                 "Data type not coded for...");
      }
    }
  }
}

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<int>& dimensions) {
  oops::Log::debug() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  oops::Log::info() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::string fieldName = field.name();
    atlas::array::DataType atlasType = field.datatype();
    int fieldSize = utilsatlas::getDataSize(field);
    switch (atlasType.kind()) {
    case atlasType.KIND_INT32: {
      if (dataContainer == nullptr) {
        dataContainer = std::make_shared<DataContainerInt>(fieldName);
      }
      std::shared_ptr<DataContainerInt> dataContainerInt =
                        std::static_pointer_cast<DataContainerInt>(dataContainer);
      dataContainerInt->clear();
      dataContainerInt->setSize(fieldSize);
      populateDataVec(dataContainerInt->getData(), field, dimensions);
      break;
    }
    case atlasType.KIND_REAL32: {
      if (dataContainer == nullptr) {
        dataContainer = std::make_shared<DataContainerFloat>(fieldName);
      }
      std::shared_ptr<DataContainerFloat> dataContainerFloat =
                        std::static_pointer_cast<DataContainerFloat>(dataContainer);
      dataContainerFloat->clear();
      dataContainerFloat->setSize(fieldSize);
      populateDataVec(dataContainerFloat->getData(), field, dimensions);
      break;
    }
    case atlasType.KIND_REAL64: {
      if (dataContainer == nullptr) {
        dataContainer = std::make_shared<DataContainerDouble>(fieldName);
      }
      std::shared_ptr<DataContainerDouble> dataContainerDouble =
                        std::static_pointer_cast<DataContainerDouble>(dataContainer);
      dataContainerDouble->clear();
      dataContainerDouble->setSize(fieldSize);
      populateDataVec(dataContainerDouble->getData(), field, dimensions);
      break;
    }
    default:
      utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                               "Data type not coded for...");
    }
  }
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                         const atlas::Field& field,
                                         const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, lfricToAtlasMap);
  data.addContainer(dataContainer);
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                         const atlas::Field& field,
                                         const std::vector<int> dimensions) {
  oops::Log::debug() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, dimensions);
  data.addContainer(dataContainer);
}

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                   const atlas::Field& field,
                                   const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateDataVec() " << field.name() << std::endl;
  int numLevels = field.levels();
  if ((lfricToAtlasMap.size() * numLevels) != dataVec.size()) {
    oops::Log::info() << "lfricToAtlasMap.size()> " << lfricToAtlasMap.size() << ", numLevels> " << numLevels << ", dataVec.size()> " << dataVec.size() << std::endl;
    utils::throwException("AtlasWriter::populateDataVec()> "
                             "Data container is not configured for the expected data...");
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
    for (int j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasWriter::populateDataVec<double>(std::vector<double>& dataVec,
                                                    const atlas::Field& field,
                                                    const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasWriter::populateDataVec<float>(std::vector<float>& dataVec,
                                                   const atlas::Field& field,
                                                   const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasWriter::populateDataVec<int>(std::vector<int>& dataVec,
                                                 const atlas::Field& field,
                                                 const std::vector<size_t>& lfricToAtlasMap);

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                   const atlas::Field& field,
                                   const std::vector<int>& dimensions) {
  oops::Log::debug() << "AtlasWriter::populateDataVec()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (int i = 0; i < dimensions[consts::eHorizontal]; ++i) {  // Horizontal dimension
    for (int j = 0; j < dimensions[consts::eVertical]; ++j) {  // Levels dimension
      int index = j + (i * dimensions[consts::eVertical]);
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasWriter::populateDataVec<double>(std::vector<double>& dataVec,
                                                    const atlas::Field& field,
                                                    const std::vector<int>& dimensions);
template void monio::AtlasWriter::populateDataVec<float>(std::vector<float>& dataVec,
                                                   const atlas::Field& field,
                                                   const std::vector<int>& dimensions);
template void monio::AtlasWriter::populateDataVec<int>(std::vector<int>& dataVec,
                                                 const atlas::Field& field,
                                                 const std::vector<int>& dimensions);
