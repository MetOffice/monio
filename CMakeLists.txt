cmake_minimum_required(VERSION 3.5)

project(monio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
FileData.cc
FileData.h
Metadata.cc
Metadata.h
Monio.cc
Monio.h
Reader.cc
Reader.h
Variable.cc
Variable.h
Writer.cc
Writer.h
)

ecbuild_add_library( TARGET     ${PROJECT_NAME}
                     SOURCES    ${monio_sources}
                     INSTALL_HEADERS LISTED
                     HEADER_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                     LINKER_LANGUAGE CXX )
