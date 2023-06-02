###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND monio_src
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
set(MONIO_SRC_DEP ${monio_src} )

ecbuild_add_library(TARGET monio
                    SOURCES ${monio_src}
                    PUBLIC_LIBS ${MONIO_LIB_DEP}
                    INCLUDES ${MONIO_INCLUDES}
                    INSTALL_HEADERS LISTED
                    LINKER_LANGUAGE ${OOPS_LINKER_LANGUAGE})

