
/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
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
                                                   atlas::Field& field,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& writeName,
                                             const bool isLfricNaming) {
  oops::Log::debug() << "AtlasWriter::populateFileDataWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<size_t>& lfricAtlasMap = fileData.getLfricAtlasMap();
    // Create dimensions
    Metadata& metadata = fileData.getMetadata();
    metadata.addDimension(std::string(consts::kHorizontalName), lfricAtlasMap.size());
    metadata.addDimension(std::string(consts::kVerticalFullName), consts::kVerticalFullSize);
    metadata.addDimension(std::string(consts::kVerticalHalfName), consts::kVerticalHalfSize);

    atlas::Field writeField = getWriteField(field, writeName, fieldMetadata.noFirstLevel);
    populateMetadataWithField(metadata, writeField, fieldMetadata, writeName);
    populateDataWithField(fileData.getData(), writeField, lfricAtlasMap, writeName);
    addGlobalAttributes(metadata, isLfricNaming);
  }
}

void monio::AtlasWriter::populateFileDataWithField(FileData& fileData,
                                             const atlas::Field& field,
                                             const std::string& writeName) {
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
    populateMetadataWithField(metadata, field, writeName);
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
    addGlobalAttributes(metadata, false);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& varName) {
  oops::Log::debug() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  // Variable dimensions
  addVariableDimensions(field, metadata, var);
  // Variable attributes
  for (int i = 0; i < consts::eNumberOfAttributeNames; ++i) {
    std::string attributeName = std::string(consts::kIncrementAttributeNames[i]);
    std::string attributeValue;
    switch (i) {
      case consts::eStandardName:
        attributeValue = fieldMetadata.jediName;
        break;
      case consts::eLongName:
        attributeValue = fieldMetadata.jediName + "_inc";
        break;
      case consts::eUnitsName:
        attributeValue = fieldMetadata.units;
        break;
      default:
        attributeValue = consts::kIncrementVariableValues[i];
    }
    std::shared_ptr<AttributeBase> incAttr = std::make_shared<AttributeString>(attributeName,
                                                                               attributeValue);
    var->addAttribute(incAttr);
  }
  metadata.addVariable(varName, var);
}

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const std::string& varName) {
  oops::Log::debug() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  // Variable dimensions
  addVariableDimensions(field, metadata, var);
  metadata.addVariable(varName, var);
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                         const atlas::Field& field,
                                         const std::vector<size_t>& lfricToAtlasMap,
                                         const std::string& fieldName) {
  oops::Log::debug() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, lfricToAtlasMap, fieldName);
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

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<size_t>& lfricToAtlasMap,
                               const std::string& fieldName) {
  oops::Log::debug() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    atlas::array::DataType atlasType = field.datatype();
    int fieldSize = utilsatlas::getGlobalDataSize(field);
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
    int fieldSize = utilsatlas::getGlobalDataSize(field);
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
    default: {
        utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                                 "Data type not coded for...");
      }
    }
  }
}

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                   const atlas::Field& field,
                                   const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateDataVec() " << field.name() << std::endl;
  int numLevels = field.levels();
  if ((lfricToAtlasMap.size() * numLevels) != dataVec.size()) {
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
  for (int i = 0; i < dimensions[consts::eHorizontal]; ++i) {
    for (int j = 0; j < dimensions[consts::eVertical]; ++j) {
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

atlas::Field monio::AtlasWriter::getWriteField(atlas::Field& field,
                                         const std::string& writeName,
                                         const bool noFirstLevel) {
  oops::Log::debug() << "AtlasWriter::getWriteField()" << std::endl;
  atlas::FunctionSpace functionSpace = field.functionspace();
  atlas::array::DataType atlasType = field.datatype();
  if (atlasType != atlasType.KIND_REAL64 &&
      atlasType != atlasType.KIND_REAL32 &&
      atlasType != atlasType.KIND_INT32) {
      utils::throwException("AtlasWriter::getWriteField())> Data type not coded for...");
  }
  // Erroneous case. For noFirstLevel == true field should have 70 levels
  if (noFirstLevel == true && field.levels() == consts::kVerticalFullSize) {
    utils::throwException("AtlasWriter::getWriteField()> Field levels misconfiguration...");
  }
  // WARNING - This name-check is an LFRic-Lite specific convention...
  if (utils::findInVector(consts::kMissingVariableNames, writeName) == false) {
    if (noFirstLevel == true && field.levels() == consts::kVerticalHalfSize) {
      atlas::util::Config atlasOptions = atlas::option::name(writeName) |
                                         atlas::option::global(0) |
                                         atlas::option::levels(consts::kVerticalFullSize);
      switch (atlasType.kind()) {
        case atlasType.KIND_REAL64: {
          return copySurfaceLevel<double>(field, functionSpace, atlasOptions);
        }
        case atlasType.KIND_REAL32: {
          return copySurfaceLevel<float>(field, functionSpace, atlasOptions);
        }
        case atlasType.KIND_INT32: {
          return copySurfaceLevel<int>(field, functionSpace, atlasOptions);
        }
      }
    } else {
      field.metadata().set("name", writeName);
    }
  } else {
    utils::throwException("AtlasWriter::getWriteField()> Field write name misconfiguration...");
  }
  return field;
}

template<typename T>
atlas::Field monio::AtlasWriter::copySurfaceLevel(const atlas::Field& inputField,
                              const atlas::FunctionSpace& functionSpace,
                              const atlas::util::Config& atlasOptions) {
  oops::Log::debug() << "AtlasWriter::copySurfaceLevel()" << std::endl;
  atlas::Field copiedField = functionSpace.createField<T>(atlasOptions);
  auto copiedFieldView = atlas::array::make_view<T, 2>(copiedField);
  auto inputFieldView = atlas::array::make_view<T, 2>(inputField);
  std::vector<int> dimVec = inputField.shape();
  for (int j = 0; j < dimVec[consts::eVertical]; ++j) {
    for (int i = 0; i < dimVec[consts::eHorizontal]; ++i) {
      copiedFieldView(i, j + 1) = inputFieldView(i, j);
    }
  }
  // Copy surface level of input field
  for (int i = 0; i < dimVec[consts::eHorizontal]; ++i) {
    copiedFieldView(i, 0) = inputFieldView(i, 0);
  }
  return copiedField;
}

template atlas::Field monio::AtlasWriter::copySurfaceLevel<double>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);
template atlas::Field monio::AtlasWriter::copySurfaceLevel<float>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);
template atlas::Field monio::AtlasWriter::copySurfaceLevel<int>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);

void monio::AtlasWriter::addVariableDimensions(const atlas::Field& field,
                                               const Metadata& metadata,
                                               std::shared_ptr<monio::Variable> var) {
  std::vector<int> dimVec = field.shape();
  if (field.metadata().get<bool>("global") == false) {
    dimVec[0] = utilsatlas::getHorizontalSize(field);  // If so, get the 2D size of the Field
  }
  // Reversal of dims required for LFRic files. Currently applied to all output files.
  std::reverse(dimVec.begin(), dimVec.end());
  for (auto& dimSize : dimVec) {
    std::string dimName = metadata.getDimensionName(dimSize);
    if (dimName != consts::kNotFoundError) {  // Not used for 1-D fields.
      var->addDimension(dimName, dimSize);
    }
  }
}

void monio::AtlasWriter::addGlobalAttributes(Metadata& metadata, const bool isLfricNaming) {
  // Initialise variables
  std::string namingConvention =
      isLfricNaming == true ? consts::kNamingConventions[consts::eLfricNaming] :
                              consts::kNamingConventions[consts::eJediNaming];
  // Create attribute objects
  std::shared_ptr<monio::AttributeString> namingAttr =
      std::make_shared<AttributeString>(std::string(consts::kNamingConventionName),
                                        namingConvention);
  std::shared_ptr<monio::AttributeString> producedByAttr =
      std::make_shared<AttributeString>(std::string(consts::kProducedByName),
                                        std::string(consts::kProducedByString));
  // Add to metadata
  metadata.addGlobalAttr(namingAttr->getName(), namingAttr);
  metadata.addGlobalAttr(producedByAttr->getName(), producedByAttr);
}
