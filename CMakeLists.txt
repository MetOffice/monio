cmake_minimum_required(VERSION 3.5)

project(monio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

include(${CMAKE_SOURCE_DIR}/src/CMakeLists.txt)

ecbuild_add_library( TARGET     ${PROJECT_NAME}
                     SOURCES    ${monio_sources}
                     INSTALL_HEADERS LISTED
                     HEADER_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                     LINKER_LANGUAGE CXX )
