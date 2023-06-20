###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
ecbuild_generate_config_headers( DESTINATION ${INSTALL_INCLUDE_DIR}/monio )

list(APPEND monio_src_files
monio/AtlasProcessor.cc
monio/AtlasProcessor.h
monio/AtlasReader.cc
monio/AtlasReader.h
monio/AtlasWriter.cc
monio/AtlasWriter.h
monio/AttributeBase.cc
monio/AttributeBase.h
monio/AttributeInt.cc
monio/AttributeInt.h
monio/AttributeString.cc
monio/AttributeString.h
monio/Constants.h
monio/Data.cc
monio/Data.h
monio/DataContainerBase.cc
monio/DataContainerBase.h
monio/DataContainerDouble.cc
monio/DataContainerDouble.h
monio/DataContainerFloat.cc
monio/DataContainerFloat.h
monio/DataContainerInt.cc
monio/DataContainerInt.h
monio/File.cc
monio/File.h
monio/FileData.cc
monio/FileData.h
monio/Metadata.cc
monio/Metadata.h
monio/Monio.cc
monio/Monio.h
monio/Reader.cc
monio/Reader.h
monio/Variable.cc
monio/Variable.h
monio/Writer.cc
monio/Writer.h
)

set(MONIO_LIB_DEP oops atlas NetCDF::NetCDF_CXX MPI::MPI_CXX)

ecbuild_add_library( TARGET ${PROJECT_NAME}
                     SOURCES ${monio_src_files}
                     PUBLIC_LIBS ${MONIO_LIB_DEP}
                     INSTALL_HEADERS LISTED
                     HEADER_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                     LINKER_LANGUAGE CXX )

## Include paths
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
                                                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
