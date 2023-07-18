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
#include "AttributeString.h"
#include "oops/util/Logger.h"

#include "Metadata.h"
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
    atlasProcessor_(mpiCommunicator, mpiRankOwner),
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::debug() << "AtlasWriter::AtlasWriter()" << std::endl;
}


void monio::AtlasWriter::writeFieldSetToFile(const atlas::FieldSet& fieldSet,
                                             const std::string outputFilePath) {
  oops::Log::debug() << "AtlasWriter::writeFieldSetToFile()" << std::endl;
  if (atlas::mpi::rank() == monio::consts::kMPIRankOwner) {
    if (outputFilePath.length() != 0) {
      monio::Metadata metadata;
      monio::Data data;
      monio::AtlasWriter atlasWriter(atlas::mpi::comm(),
                                         monio::consts::kMPIRankOwner);
      atlasWriter.populateMetadataAndDataWithFieldSet(metadata, data, fieldSet);

      monio::Writer writer(atlas::mpi::comm(),
                               monio::consts::kMPIRankOwner,
                               outputFilePath);
      writer.writeMetadata(metadata);
      writer.writeVariablesData(metadata, data);
    } else {
      oops::Log::info() << "AtlasWriter::writeFieldSetToFile() No outputFilePath supplied. "
                           "NetCDF writing will not take place." << std::endl;
    }
  }
}

void monio::AtlasWriter::writeIncrementsToFile(
                                  atlas::FieldSet& fieldSet,
                            const std::vector<std::string>& varNames,
                            const std::map<std::string, consts::FieldMetadata>& fieldMetadataMap,
                                  monio::FileData& fileData,
                            const std::string& outputFilePath) {
  oops::Log::debug() << "AtlasWriter::writeFieldSetToFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (outputFilePath.length() != 0) {
        monio::Metadata& readMetadata = fileData.getMetadata();
        monio::Data& readData = fileData.getData();
        std::vector<size_t>& lfricAtlasMap = fileData.getLfricAtlasMap();

        readMetadata.clearGlobalAttributes();

        readMetadata.deleteDimension(std::string(monio::consts::kTimeDimName));
        readMetadata.deleteDimension(std::string(monio::consts::kTileDimName));

        readData.deleteContainer(std::string(monio::consts::kTimeVarName));
        readData.deleteContainer(std::string(monio::consts::kTileVarName));

        reconcileMetadataWithData(readMetadata, readData);

        // Add data and metadata for increments in fieldSet
        populateMetadataAndDataWithLfricFieldSet(readMetadata, readData, varNames,
                                                 fieldMetadataMap, fieldSet, lfricAtlasMap);

        monio::Writer writer(atlas::mpi::comm(),
                             monio::consts::kMPIRankOwner,
                             outputFilePath);
        writer.writeMetadata(readMetadata);
        writer.writeVariablesData(readMetadata, readData);
    } else {
      oops::Log::info() << "AtlasWriter::writeFieldSetToFile() No outputFilePath supplied. "
                           "NetCDF writing will not take place." << std::endl;
    }
  }
}

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const consts::FieldMetadata* fieldMetadata,
                                                   bool reverseDims) {
  oops::Log::debug() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  std::string varName = field.name();
  int type = atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  std::vector<int> dimVec = field.shape();

  // Check if Field is not global
  if (field.metadata().get<bool>("global") == false) {
    dimVec[0] = atlasProcessor_.getSizeOwned(field);  // If so, get the 'size owned' by the Fields
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
      throw std::runtime_error("AtlasWriter::populateDataContainerWithField()> "
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
      throw std::runtime_error("AtlasWriter::populateDataContainerWithField()> "
                               "Data type not coded for...");
    }
  }
}

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                         const atlas::Field& field,
                                         const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateDataVec() " << field.name() << std::endl;
  atlas::idx_t numLevels = field.levels();
  if ((lfricToAtlasMap.size() * numLevels) != dataVec.size()) {
    throw std::runtime_error("AtlasWriter::populateDataVec()> "
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

void monio::AtlasWriter::populateMetadataAndDataWithFieldSet(Metadata& metadata,
                                                                 Data& data,
                                                           const atlas::FieldSet& fieldSet) {
  oops::Log::debug() << "AtlasWriter::populateMetadataAndDataWithFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dimCount = 0;
    bool createdLonLat = false;
    for (const auto& field : fieldSet) {
      std::vector<int> dimVec = field.shape();
      if (field.metadata().get<bool>("global") == false) {
        dimVec[0] = atlasProcessor_.getSizeOwned(field);
      }
      for (auto& dimSize : dimVec) {
        std::string dimName = metadata.getDimensionName(dimSize);
        if (dimName == monio::consts::kNotFoundError) {
          dimName = "dim" + std::to_string(dimCount);
          metadata.addDimension(dimName, dimSize);
          dimCount++;
        }
      }
      populateMetadataWithField(metadata, field);
      if (createdLonLat == false) {
        std::vector<atlas::PointLonLat> atlasLonLat = atlasProcessor_.getAtlasCoords(field);
        std::vector<std::shared_ptr<DataContainerBase>> coordContainers =
                  convertLatLonToContainers(atlasLonLat, consts::kCoordVarNames);
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
      metadata.addGlobalAttr(producedByName, producedByAttr);
    }
  }
}

void monio::AtlasWriter::populateMetadataAndDataWithLfricFieldSet(
                                  Metadata& metadata,
                                  Data& data,
                            const std::vector<std::string>& varNames,
                            const std::map<std::string, consts::FieldMetadata>& fieldMetadataMap,
                                  atlas::FieldSet& fieldSet,
                            const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::debug() << "AtlasWriter::populateMetadataAndDataWithLfricFieldSet()" << std::endl;
  // Dimensions
  metadata.addDimension(std::string(consts::kHorizontalName), lfricToAtlasMap.size());
  metadata.addDimension(std::string(consts::kVerticalFullName), consts::kVerticalFullSize);
  metadata.addDimension(std::string(consts::kVerticalHalfName), consts::kVerticalHalfSize);
  // Variables
  for (auto& field : fieldSet) {
    std::string fieldName = field.name();
    if (std::find(varNames.begin(), varNames.end(), fieldName) != varNames.end()) {
      consts::FieldMetadata fieldMetadata = fieldMetadataMap.at(fieldName);
      atlas::Field processedField = processField(field, fieldMetadata);
      populateMetadataWithField(metadata, processedField, &fieldMetadata, true);
      size_t totalFieldSize = metadata.getVariable(processedField.name())->getTotalSize();
      populateDataWithField(data, processedField, lfricToAtlasMap, totalFieldSize);
    }
  }
  // Global attrs
  std::string producedByName = "Produced_by";
  std::string iLoveItWhenName = "I_love_it_when";
  std::string inAssociationWithName = "In_association_with";
  std::string producedByString =
      "The DA Team: Lorenzo, Marek, Oliver, Phil, Rick, Stefano, and many more!";
  std::string iLoveItWhenString = "A plan comes together.";
  std::string inAssociationWithString = "Wlasak`s Tiles®";

  std::shared_ptr<monio::AttributeString> producedByAttr =
          std::make_shared<AttributeString>(producedByName, producedByString);
  std::shared_ptr<monio::AttributeString> iLoveItWhenAttr =
          std::make_shared<AttributeString>(iLoveItWhenName, iLoveItWhenString);
  std::shared_ptr<monio::AttributeString> inAssociationWithAttr =
          std::make_shared<AttributeString>(inAssociationWithName, inAssociationWithString);
  metadata.addGlobalAttr(producedByName, producedByAttr);
  metadata.addGlobalAttr(iLoveItWhenName, iLoveItWhenAttr);
  metadata.addGlobalAttr(inAssociationWithName, inAssociationWithAttr);
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

void monio::AtlasWriter::reconcileMetadataWithData(Metadata& metadata, Data& data) {
  oops::Log::debug() << "AtlasWriter::reconcileMetadataWithData()" << std::endl;
  std::vector<std::string> metadataVarNames = metadata.getVariableNames();
  std::vector<std::string> dataContainerNames = data.getDataContainerNames();

  for (const auto& metadataVarName : metadataVarNames) {
    auto it = std::find(begin(dataContainerNames), end(dataContainerNames), metadataVarName);
    if (it == std::end(dataContainerNames)) {
      metadata.deleteVariable(metadataVarName);
    }
  }
}

std::vector<std::shared_ptr<monio::DataContainerBase>>
        monio::AtlasWriter::convertLatLonToContainers(
                        const std::vector<atlas::PointLonLat>& atlasCoords,
                        const std::vector<std::string>& coordNames) {
  std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
  std::shared_ptr<DataContainerDouble> lonContainer =
            std::make_shared<DataContainerDouble>(coordNames[consts::eLongitude]);
  std::shared_ptr<DataContainerDouble> latContainer =
            std::make_shared<DataContainerDouble>(coordNames[consts::eLatitude]);
  for (const auto& atlasCoord : atlasCoords) {
    lonContainer->setDatum(atlasCoord.lon());
    latContainer->setDatum(atlasCoord.lat());
  }
  coordContainers.push_back(lonContainer);
  coordContainers.push_back(latContainer);
  return coordContainers;
}

atlas::Field monio::AtlasWriter::processField(atlas::Field& inputField,
                                        const consts::FieldMetadata& fieldMetadata) {
  oops::Log::debug() << "AtlasWriter::processField()" << std::endl;
  atlas::FunctionSpace functionSpace = inputField.functionspace();
  atlas::array::DataType atlasType = inputField.datatype();

  if (fieldMetadata.lfricWriteName != "TO BE DERIVED" &&
      fieldMetadata.lfricWriteName != "TO BE IMPLEMENTED") {
    if (fieldMetadata.copyFirstLevel == true) {
      atlas::util::Config atlasOptions = atlas::option::name(fieldMetadata.lfricWriteName) |
                                         atlas::option::global(0) |
                                         atlas::option::levels(inputField.levels() + 1);
      switch (atlasType.kind()) {
        case atlasType.KIND_REAL64: {
          return copySurfaceLevel<double>(inputField, functionSpace, atlasOptions);
        }
        case atlasType.KIND_REAL32: {
          return copySurfaceLevel<float>(inputField, functionSpace, atlasOptions);
        }
        case atlasType.KIND_INT32: {
          return copySurfaceLevel<int>(inputField, functionSpace, atlasOptions);
        }
      }
    } else {
      inputField.metadata().set("name", fieldMetadata.lfricWriteName);
    }
  }
  return inputField;
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
  for (atlas::idx_t j = 0; j < dimVec[consts::eVertical]; ++j) {
    for (atlas::idx_t i = 0; i < dimVec[consts::eHorizontal]; ++i) {
      copiedFieldView(i, j + 1) = inputFieldView(i, j);
    }
  }
  // Copy surface level of input field
  for (int i = 0; i < dimVec[consts::eHorizontal]; ++i) {
      copiedFieldView(i, 0) = inputFieldView(i, 0);
  }
  return copiedField;
}

template atlas::Field
    monio::AtlasWriter::copySurfaceLevel<double>(const atlas::Field& inputField,
                                                     const atlas::FunctionSpace& functionSpace,
                                                     const atlas::util::Config& atlasOptions);
template atlas::Field
    monio::AtlasWriter::copySurfaceLevel<float>(const atlas::Field& inputField,
                                                    const atlas::FunctionSpace& functionSpace,
                                                    const atlas::util::Config& atlasOptions);
template atlas::Field
    monio::AtlasWriter::copySurfaceLevel<int>(const atlas::Field& inputField,
                                                  const atlas::FunctionSpace& functionSpace,
                                                  const atlas::util::Config& atlasOptions);

int monio::AtlasWriter::atlasTypeToMonioEnum(atlas::array::DataType atlasType) {
  oops::Log::debug() << "AtlasWriter::atlasTypeToMonioEnum()" << std::endl;
  switch (atlasType.kind()) {
  case atlasType.KIND_INT32: {
    return consts::eInt;
  }
  case atlasType.KIND_REAL32: {
    return consts::eFloat;
  }
  case atlasType.KIND_REAL64: {
    return consts::eDouble;
  }
  default:
    throw std::runtime_error("AtlasWriter::fromFieldSet()> Data type not coded for...");
  }
}
