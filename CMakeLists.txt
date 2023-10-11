###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023, Met Office. All rights reserved.                  #
#                                                                             #
# This software is licensed under the terms of the 3-Clause BSD License       #
# which can be obtained from https://opensource.org/license/bsd-3-clause/.    #
###############################################################################

cmake_minimum_required(VERSION 3.12)

project(monio VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH})
set(CMAKE_DIRECTORY_LABELS ${PROJECT_NAME})

## Ecbuild integration
set(ECBUILD_DEFAULT_BUILD_TYPE Release)
set(ENABLE_MPI ON CACHE BOOL "Compile with MPI")

find_package(ecbuild)
include(ecbuild_system NO_POLICY_SCOPE)
ecbuild_requires_macro_version(2.7)

## Project
ecbuild_declare_project()
set(MONIO_LINKER_LANGUAGE CXX)

## Dependencies
find_package(jedicmake REQUIRED)
find_package(MPI REQUIRED COMPONENTS CXX)
find_package(HDF5 REQUIRED COMPONENTS)
find_package(NetCDF COMPONENTS CXX)
find_package(eckit 1.16.1 REQUIRED COMPONENTS MPI)
find_package(atlas 0.20.2 REQUIRED)
find_package(oops 1.0.0 REQUIRED)

## Export package info
#list(APPEND MPI HDF5 NetCDF atlas oops)

set(MONIO_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
                       ${CMAKE_CURRENT_BINARY_DIR}/src
                       ${oops_SOURCE_DIR}/src)

set(MONIO_LIBRARIES monio)
get_directory_property(MONIO_DEFINITIONS COMPILE_DEFINITIONS)

## Sources
include(monio_compiler_flags)
include_directories(${MONIO_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_subdirectory(src)
add_subdirectory(test)

## Finalise configuration
ecbuild_install_project(NAME ${PROJECT_NAME})
ecbuild_print_summary()
