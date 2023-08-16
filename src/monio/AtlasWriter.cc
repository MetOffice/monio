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

namespace {
  template<typename T>
  T productOfVector(const std::vector<T>& vectorToMultiply) {
    T sum = 1;
    std::for_each(begin(vectorToMultiply), end(vectorToMultiply), [&](T elem) {
      sum *= elem;
    });
    return sum;
  }
}  // anonymous namespace

monio::AtlasWriter::AtlasWriter(const eckit::mpi::Comm& mpiCommunicator,
                                    const atlas::idx_t mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasWriter::AtlasWriter()" << std::endl;
}

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const consts::FieldMetadata* fieldMetadata,
                                                   bool reverseDims) {
  oops::Log::debug() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  std::string varName = field.name();
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  std::vector<int> dimVec = field.shape();

  // Check if Field is not global
  if (field.metadata().get<bool>("global") == false) {
    dimVec[0] = utilsatlas::getSizeOwned(field);  // If so, get the 'size owned' by the Fields
  }
  if (reverseDims == true) {
    std::reverse(dimVec.begin(), dimVec.end());

    // Reversal of dims applies to LFRic files. Using that flag here for increment attributes.
    if (fieldMetadata != nullptr) {
      for (int i = 0; i < consts::eNumberOfAttributeNames; ++i) {
        std::string attributeName = std::string(consts::kIncrementAttributeNames[i]);
        std::string attributeValue;
        switch (i) {
          case consts::eStandardName:
            attributeValue = fieldMetadata->jediName;
            break;
          case consts::eLongName:
            attributeValue = fieldMetadata->jediName + "_inc";
            break;
          case consts::eUnitsName:
            attributeValue = fieldMetadata->units;
            break;
          default:
            attributeValue = consts::kIncrementVariableValues[i];
        }
        std::shared_ptr<AttributeBase> incAttr = std::make_shared<AttributeString>(attributeName,
                                                                                   attributeValue);
        var->addAttribute(incAttr);
      }
    }
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
                               const std::vector<size_t>& lfricToAtlasMap,
                               const size_t fieldSize) {
  oops::Log::debug() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::string fieldName = field.name();
    atlas::array::DataType atlasType = field.datatype();

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
    default:
      utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                               "Data type not coded for...");
    }
  }
}

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<int>& dimensions) {
  oops::Log::debug() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::string fieldName = field.name();
    atlas::array::DataType atlasType = field.datatype();
    int fieldSize = productOfVector(dimensions);
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

void monio::AtlasWriter::populateFileDataWithFieldSet(FileData& fileData,
                                                const atlas::FieldSet& fieldSet) {
  oops::Log::debug() << "AtlasWriter::populateMetadataAndDataWithFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    Metadata& metadata = fileData.getMetadata();
    Data& data = fileData.getData();
    int dimCount = 0;
    bool createdLonLat = false;
    for (const auto& field : fieldSet) {
      std::vector<int> dimVec = field.shape();
      if (field.metadata().get<bool>("global") == false) {
        dimVec[0] = utilsatlas::getSizeOwned(field);
      }
      for (auto& dimSize : dimVec) {
        std::string dimName = metadata.getDimensionName(dimSize);
        if (dimName == consts::kNotFoundError) {
          dimName = "dim" + std::to_string(dimCount);
          fileData.getMetadata().addDimension(dimName, dimSize);
          dimCount++;
        }
      }
      populateMetadataWithField(metadata, field);
      if (createdLonLat == false) {
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
        createdLonLat = true;
      }
      populateDataWithField(data, field, metadata.getVariable(field.name())->getDimensionsVec());
      // Global attrs
      std::string producedByName = "Produced_by";
      std::string producedByString = "MONIO: Met Office NetCDF I/O";

      std::shared_ptr<monio::AttributeString> producedByAttr =
              std::make_shared<AttributeString>(producedByName, producedByString);
      fileData.getMetadata().addGlobalAttr(producedByName, producedByAttr);
    }
  }
}

void monio::AtlasWriter::populateFileDataWithLfricFieldSet(FileData& fileData,
                                         const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                                               atlas::FieldSet& fieldSet,
                                         const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateMetadataAndDataWithLfricFieldSet()" << std::endl;
  Metadata& metadata = fileData.getMetadata();
  Data& data = fileData.getData();
  // Dimensions
  metadata.addDimension(std::string(consts::kHorizontalName), lfricToAtlasMap.size());
  metadata.addDimension(std::string(consts::kVerticalFullName), consts::kVerticalFullSize);
  metadata.addDimension(std::string(consts::kVerticalHalfName), consts::kVerticalHalfSize);
  // Variables
  for (const auto& fieldMetadata : fieldMetadataVec) {
    atlas::Field& field = fieldSet.field(fieldMetadata.jediName);
    atlas::Field processedField = utilsatlas::getFormattedField(field, fieldMetadata);
    populateMetadataWithField(metadata, processedField, &fieldMetadata, true);
    size_t totalFieldSize = metadata.getVariable(processedField.name())->getTotalSize();
    populateDataWithField(data, processedField, lfricToAtlasMap, totalFieldSize);
  }
  // Global attrs
  std::string producedByName = "Produced_by";
  std::string producedByString = "MONIO: Met Office NetCDF I/O";

  std::shared_ptr<monio::AttributeString> producedByAttr =
          std::make_shared<AttributeString>(producedByName, producedByString);
  fileData.getMetadata().addGlobalAttr(producedByName, producedByAttr);
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                             const atlas::Field& field,
                                             const std::vector<size_t>& lfricToAtlasMap,
                                             const size_t totalFieldSize) {
  oops::Log::debug() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, lfricToAtlasMap, totalFieldSize);
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
  atlas::idx_t numLevels = field.levels();
  if ((lfricToAtlasMap.size() * numLevels) != dataVec.size()) {
    utils::throwException("AtlasWriter::populateDataVec()> "
                             "Data container is not configured for the expected data...");
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
    for (atlas::idx_t j = 0; j < numLevels; ++j) {
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
  for (atlas::idx_t i = 0; i < dimensions[consts::eHorizontal]; ++i) {  // Horizontal dimension
    for (atlas::idx_t j = 0; j < dimensions[consts::eVertical]; ++j) {  // Levels dimension
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
