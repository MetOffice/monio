/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "lfriclitejedi/IO/AtlasProcessor.h"

#include <algorithm>
#include <numeric>

#include "AttributeString.h"

#include "atlas/functionspace.h"
#include "atlas/grid/Iterator.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/util/KDTree.h"
#include "lfriclitejedi/IO/Metadata.h"
#include "lfriclitejedi/IO/Writer.h"
#include "oops/util/Logger.h"

monio::AtlasProcessor::AtlasProcessor(const eckit::mpi::Comm& mpiCommunicator,
                                      const atlas::idx_t& mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "AtlasProcessor::AtlasProcessor()" << std::endl;
}

void monio::AtlasProcessor::writeFieldSetToFile(atlas::FieldSet fieldSet,
                                                std::string outputFilePath) {
  oops::Log::trace() << "AtlasProcessor::writeFieldSetToFile()" << std::endl;
  monio::Metadata metadata;
  monio::Data data;
  monio::AtlasProcessor atlasProcessor(atlas::mpi::comm(), monio::constants::kMPIRankOwner);
  atlasProcessor.populateMetadataAndDataWithFieldSet(metadata, data, fieldSet);

  monio::Writer writer(atlas::mpi::comm(), monio::constants::kMPIRankOwner, outputFilePath);
  writer.writeMetadata(metadata);
  writer.writeVariablesData(metadata, data);
}

void monio::AtlasProcessor::createGlobalFieldSet(const atlas::FieldSet& localFieldSet,
                                                       atlas::FieldSet& globalFieldSet) {
  oops::Log::trace() << "AtlasProcessor::createGlobalFieldSet()" << std::endl;
  for (const auto& localField : localFieldSet) {
    std::string localFieldName = localField.name();
    int levels = localField.levels();
    atlas::util::Config atlasOptions = atlas::option::name(localFieldName) |
          atlas::option::levels(levels) |
          atlas::option::global(0);

    const auto& functionSpace = localField.functionspace();
    atlas::array::DataType atlasType = localField.datatype();
    atlas::Field globalField;
    switch (atlasType.kind()) {
    case atlasType.KIND_INT32: {
      globalField = functionSpace.createField<int>(atlasOptions);
      break;
    }
    case atlasType.KIND_REAL32: {
      globalField = functionSpace.createField<float>(atlasOptions);
      break;
    }
    case atlasType.KIND_REAL64: {
      globalField = functionSpace.createField<double>(atlasOptions);
      break;
    }
    default:
      throw std::runtime_error("AtlasProcessor::fromFieldSet()> Data type not coded for...");
    }
    localField.haloExchange();
    functionSpace.gather(localField, globalField);
    globalFieldSet.add(globalField);
  }
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getAtlasCoords(const atlas::Field field) {
  oops::Log::trace() << "AtlasProcessor::getAtlasCoords()" << std::endl;
  std::vector<atlas::PointLonLat> atlasCoords;
  auto grid = atlas::functionspace::NodeColumns(field.functionspace()).mesh().grid();
  for (auto lonLatIt = grid.lonlat().begin(); lonLatIt != grid.lonlat().end(); ++lonLatIt) {
    atlasCoords.push_back(*lonLatIt);
  }
  return atlasCoords;
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getAtlasCoords(
                                                            const atlas::CubedSphereGrid& grid) {
  oops::Log::trace() << "AtlasProcessor::getAtlasCoords()" << std::endl;
  std::vector<atlas::PointLonLat> atlasCoords;
  atlasCoords.resize(grid.size());
  std::copy(grid.lonlat().begin(), grid.lonlat().end(), atlasCoords.begin());
  return atlasCoords;
}

std::vector<atlas::PointLonLat> monio::AtlasProcessor::getLfricCoords(
                     const std::vector<std::shared_ptr<monio::DataContainerBase>>& coordData) {
  oops::Log::trace() << "AtlasProcessor::getLfricCoords()" << std::endl;

  std::vector<atlas::PointLonLat> lfricCoords;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (coordData.size() == 2) {
      std::array<std::vector<float>, 2> coordVectorArray;
      int coordCount = 0;
      for (auto& coordContainer : coordData) {
        // LFRic coordinate data are currently stored as floats
        if (coordContainer->getType() == constants::eFloat) {
          std::shared_ptr<DataContainerFloat> cooordContainerFloat =
                    std::static_pointer_cast<DataContainerFloat>(coordContainer);
          std::vector<float> coordData = cooordContainerFloat->getData();

          coordVectorArray[coordCount] = coordData;
          coordCount++;
        } else {
          throw std::runtime_error("AtlasProcessor::getLfricCoords()> "
              "Data type not coded for...");
        }
      }
      // Populate Atlas PointLonLat vector
      lfricCoords.reserve(coordVectorArray[0].size());
      for (auto lonIt = coordVectorArray[0].begin(), latIt = coordVectorArray[1].begin();
                                    lonIt != coordVectorArray[0].end(); ++lonIt , ++latIt) {
        lfricCoords.push_back(atlas::PointLonLat(*lonIt, *latIt));
      }
    } else {
        throw std::runtime_error("AtlasProcessor::getLfricCoords()> "
            "Incorrect number of coordinate axes...");
    }
  }
  return lfricCoords;
}

std::vector<size_t> monio::AtlasProcessor::createLfricAtlasMap(
                                            const std::vector<atlas::PointLonLat>& atlasCoords,
                                            const std::vector<atlas::PointLonLat>& lfricCoords) {
  oops::Log::trace() << "AtlasProcessor::createLfricAtlasMap()" << std::endl;
  std::vector<size_t> lfricAtlasMap;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    // Essential check to ensure grid is configured to accommodate the data
    if (atlasCoords.size() != lfricCoords.size()) {
      throw std::runtime_error("AtlasProcessor::createLfricAtlasMap()> "
        "Configured grid is not compatible with input file...");
    }
    lfricAtlasMap.reserve(atlasCoords.size());

    // Make a kd-tree using atlasLonLat as the point,
    // with element index i as payload
    std::vector<size_t> indices(atlasCoords.size());
    std::iota(begin(indices), end(indices), 0);

    const atlas::Geometry unitSphere(1.0);
    atlas::util::IndexKDTree tree(unitSphere);
    tree.build(atlasCoords, indices);

    // Partitioning vector to create atlas::Distribution for mesh generation
    std::vector<int> partitioning(tree.size(), -1);

    // find atlas global indices for each element of modelLonLat
    for (const auto& lfricCoord : lfricCoords) {
      auto idx = tree.closestPoint(lfricCoord).payload();
      lfricAtlasMap.push_back(idx);
    }
  }
  return lfricAtlasMap;
}

std::vector<std::shared_ptr<monio::DataContainerBase>>
        monio::AtlasProcessor::convertLatLonToContainers(
                        const std::vector<atlas::PointLonLat>& atlasCoords,
                        const std::vector<std::string>& coordNames) {
  std::vector<std::shared_ptr<monio::DataContainerBase>> coordContainers;
  std::shared_ptr<DataContainerDouble> lonContainer =
            std::make_shared<DataContainerDouble>(coordNames[constants::eLongitude]);
  std::shared_ptr<DataContainerDouble> latContainer =
            std::make_shared<DataContainerDouble>(coordNames[constants::eLatitude]);
  for (const auto& atlasCoord : atlasCoords) {
    lonContainer->setDatum(atlasCoord.lon());
    latContainer->setDatum(atlasCoord.lat());
  }
  coordContainers.push_back(lonContainer);
  coordContainers.push_back(latContainer);
  return coordContainers;
}

void monio::AtlasProcessor::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer,
                                      const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateFieldWithDataContainer()" << std::endl;
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
      throw std::runtime_error("AtlasProcessor::populateFieldWithDataContainer()> "
                               "Data type not coded for...");
    }
  }
}

void monio::AtlasProcessor::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateDataContainerWithField()" << std::endl;
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
    dataContainerInt->setSize(field.size());
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
    dataContainerFloat->setSize(field.size());
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
    dataContainerDouble->setSize(field.size());
    populateDataVec(dataContainerDouble->getData(), field, lfricToAtlasMap);
    break;
  }
  default:
    throw std::runtime_error("AtlasProcessor::populateDataContainerWithField()> "
                             "Data type not coded for...");
  }
}


void monio::AtlasProcessor::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field) {
  oops::Log::trace() << "AtlasProcessor::populateDataContainerWithField()" << std::endl;
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
    dataContainerInt->setSize(field.size());
    populateDataVec(dataContainerInt->getData(), field);
    break;
  }
  case atlasType.KIND_REAL32: {
    if (dataContainer == nullptr) {
      dataContainer = std::make_shared<DataContainerFloat>(fieldName);
    }
    std::shared_ptr<DataContainerFloat> dataContainerFloat =
                      std::static_pointer_cast<DataContainerFloat>(dataContainer);
    dataContainerFloat->clear();
    dataContainerFloat->setSize(field.size());
    populateDataVec(dataContainerFloat->getData(), field);
    break;
  }
  case atlasType.KIND_REAL64: {
    if (dataContainer == nullptr) {
      dataContainer = std::make_shared<DataContainerDouble>(fieldName);
    }
    std::shared_ptr<DataContainerDouble> dataContainerDouble =
                      std::static_pointer_cast<DataContainerDouble>(dataContainer);
    dataContainerDouble->clear();
    dataContainerDouble->setSize(field.size());
    populateDataVec(dataContainerDouble->getData(), field);
    break;
  }
  default:
    throw std::runtime_error("AtlasProcessor::populateDataContainerWithField()> "
                             "Data type not coded for...");
  }
}

template<typename T>
void monio::AtlasProcessor::populateField(atlas::Field& field,
                                    const std::vector<T>& dataVec,
                                    const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  size_t numLevels = field.levels();
  for (size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      fieldView(i, j) = dataVec[index];
    }
  }
}

template void monio::AtlasProcessor::populateField<double>(atlas::Field& field,
                                                     const std::vector<double>& dataVec,
                                                     const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasProcessor::populateField<float>(atlas::Field& field,
                                                    const std::vector<float>& dataVec,
                                                    const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasProcessor::populateField<int>(atlas::Field& field,
                                                  const std::vector<int>& dataVec,
                                                  const std::vector<size_t>& lfricToAtlasMap);

template<typename T>
void monio::AtlasProcessor::populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateDataVec()" << std::endl;
  size_t numLevels = field.levels();
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
    for (size_t j = 0; j < numLevels; ++j) {
      int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasProcessor::populateDataVec<double>(std::vector<double>& dataVec,
                                                       const atlas::Field& field,
                                                       const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasProcessor::populateDataVec<float>(std::vector<float>& dataVec,
                                                      const atlas::Field& field,
                                                      const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasProcessor::populateDataVec<int>(std::vector<int>& dataVec,
                                                    const atlas::Field& field,
                                                    const std::vector<size_t>& lfricToAtlasMap);

template<typename T>
void monio::AtlasProcessor::populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field) {
  oops::Log::trace() << "AtlasProcessor::populateDataVec()" << std::endl;
  std::vector<int> fieldShape = field.shape();
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (size_t i = 0; i < fieldShape[constants::eHorizontal]; ++i) {  // Horizontal dimension
    for (size_t j = 0; j < fieldShape[constants::eVertical]; ++j) {  // Levels dimension
      int index = i + (j * fieldShape[constants::eHorizontal]);
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasProcessor::populateDataVec<double>(std::vector<double>& dataVec,
                                                       const atlas::Field& field);
template void monio::AtlasProcessor::populateDataVec<float>(std::vector<float>& dataVec,
                                                      const atlas::Field& field);
template void monio::AtlasProcessor::populateDataVec<int>(std::vector<int>& dataVec,
                                                    const atlas::Field& field);

void monio::AtlasProcessor::populateMetadataAndDataWithFieldSet(Metadata& metadata,
                                                                Data& data,
                                                          const atlas::FieldSet& fieldSet) {
  oops::Log::trace() << "AtlasProcessor::populateMetadataAndDataWithFieldSet()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dimCount = 0;
    bool createdLonLat = false;
    for (const auto& field : fieldSet) {
      std::vector<int> dimVec = field.shape();
      for (const auto& dimSize : dimVec) {
        std::string dimName = metadata.getDimensionName(dimSize);
        if (dimName == monio::constants::kNotFoundError) {
          dimName = "dim" + std::to_string(dimCount);
          metadata.addDimension(dimName, dimSize);
          dimCount++;
        }
      }
      populateMetadataWithField(metadata, field);
      if (createdLonLat == false) {
        std::vector<atlas::PointLonLat> atlasLonLat = getAtlasCoords(field);
        std::vector<std::shared_ptr<DataContainerBase>> coordContainers =
                  convertLatLonToContainers(atlasLonLat, constants::kCoordVarNames);
        for (const auto& coordContainer : coordContainers) {
          data.addContainer(coordContainer);
        }
        std::shared_ptr<monio::Variable> lonVar = std::make_shared<Variable>(
                      constants::kCoordVarNames[constants::eLongitude], constants::eDouble);
        std::shared_ptr<monio::Variable> latVar = std::make_shared<Variable>(
                      constants::kCoordVarNames[constants::eLatitude], constants::eDouble);
        std::string dimName = metadata.getDimensionName(atlasLonLat.size());
        lonVar->addDimension(dimName, atlasLonLat.size());
        latVar->addDimension(dimName, atlasLonLat.size());
        metadata.addVariable(constants::kCoordVarNames[constants::eLongitude], lonVar);
        metadata.addVariable(constants::kCoordVarNames[constants::eLatitude], latVar);
        createdLonLat = true;
      }
      populateDataWithField(data, field);
    }
    // Global attrs
    std::string producedByName = "Produced_by";
    std::string producedByString = "MONIO: Met Office NetCDF I/O";

    std::shared_ptr<monio::AttributeString> producedByAttr =
            std::make_shared<AttributeString>(producedByName, producedByString);
    metadata.addGlobalAttr(producedByName, producedByAttr);
  }
}

void monio::AtlasProcessor::populateMetadataAndDataWithLfricFieldSet(
                                                            Metadata& metadata,
                                                            Data& data,
                                                      const atlas::FieldSet& fieldSet,
                                                      const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateMetadataAndDataWithLfricFieldSet()" << std::endl;
  // Dimensions
  metadata.addDimension(constants::kHorizontalName, lfricToAtlasMap.size());
  metadata.addDimension(constants::kVerticalFullName, constants::kVerticalFullSize);
  metadata.addDimension(constants::kVerticalHalfName, constants::kVerticalHalfSize);
  // Variables
  for (const auto& field : fieldSet) {
    populateMetadataWithField(metadata, field);
    populateDataWithField(data, field, lfricToAtlasMap);
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

void monio::AtlasProcessor::populateMetadataWithField(Metadata& metadata,
                                                const atlas::Field field) {
  oops::Log::trace() << "AtlasProcessor::populateMetadataWithField()" << std::endl;
  std::string varName = field.name();
  int type = atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);

  std::vector<int> dimVec = field.shape();
  for (const auto& dimSize : dimVec) {
    std::string dimName = metadata.getDimensionName(dimSize);
    if (dimName != constants::kNotFoundError) {  // Not used for 1-D fields.
      var->addDimension(dimName, dimSize);
    }
  }
  metadata.addVariable(varName, var);
}

void monio::AtlasProcessor::populateDataWithField(Data& data,
                                            const atlas::Field field,
                                            const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasProcessor::populateDataWithField()" << std::endl;
  atlas::array::DataType dataType = field.datatype();
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, lfricToAtlasMap);
  data.addContainer(dataContainer);
}

void monio::AtlasProcessor::populateDataWithField(Data& data,
                                            const atlas::Field field) {
  oops::Log::trace() << "AtlasProcessor::populateDataWithField()" << std::endl;
  atlas::array::DataType dataType = field.datatype();
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field);
  data.addContainer(dataContainer);
}

atlas::idx_t monio::AtlasProcessor::getSizeOwned(const atlas::Field field) {
  atlas::Field ghostField = field.functionspace().ghost();
  atlas::idx_t sizeOwned = 0;
  auto ghostView = atlas::array::make_view<int, 1>(ghostField);
  for (size_t i = 0; i < ghostField.size(); ++i) {
    if (ghostView(i) == 1) {
      sizeOwned = i;
      break;
    }
  }
  return sizeOwned;
}

int monio::AtlasProcessor::atlasTypeToMonioEnum(atlas::array::DataType atlasType) {
  switch (atlasType.kind()) {
  case atlasType.KIND_INT32: {
    return constants::eInt;
  }
  case atlasType.KIND_REAL32: {
    return constants::eFloat;
  }
  case atlasType.KIND_REAL64: {
    return constants::eDouble;
  }
  default:
    throw std::runtime_error("AtlasProcessor::fromFieldSet()> Data type not coded for...");
  }
}
