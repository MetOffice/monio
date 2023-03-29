###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

list(APPEND monio_sources
AtlasData.cc
AtlasData.h
AtlasProcessor.cc
AtlasProcessor.h
AttributeBase.cc
AttributeBase.h
AttributeInt.cc
AttributeInt.h
AttributeString.cc
AttributeString.h
Constants.h
Data.cc
Data.h
DataContainerBase.cc
DataContainerBase.h
DataContainerDouble.cc
DataContainerDouble.h
DataContainerFloat.cc
DataContainerFloat.h
DataContainerInt.cc
DataContainerInt.h
File.cc
File.h
Metadata.cc
Metadata.h
Reader.cc
Reader.h
Variable.cc
Variable.h
Writer.cc
Writer.h
)

set(MONIO_LIB_DEP atlas NetCDF::NetCDF_CXX MPI::MPI_CXX)
set(MONIO_SRC_DEP ${lfriclite_src_files} )

ecbuild_add_library( TARGET monio
                     SOURCES  ${monio_sources}
                     PUBLIC_LIBS  ${MONIO_LIB_DEP}
                     INCLUDES ${MONIO_SRC_DEP}
                     INSTALL_HEADERS LISTED
                     LINKER_LANGUAGE ${OOPS_LINKER_LANGUAGE} )
