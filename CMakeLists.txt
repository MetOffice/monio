###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(monio VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH})
set(CMAKE_DIRECTORY_LABELS ${PROJECT_NAME})

## Ecbuild integration
set(ECBUILD_DEFAULT_BUILD_TYPE Release)
find_package( ecbuild 3.3.2 REQUIRED )
include(ecbuild_system NO_POLICY_SCOPE)
ecbuild_declare_project()

include(monio_compiler_flags)

## Dependencies
find_package(MPI REQUIRED COMPONENTS CXX Fortran)
find_package(NetCDF COMPONENTS CXX)
find_package(HDF5 REQUIRED COMPONENTS)
find_package(eckit 1.16.1 REQUIRED COMPONENTS MPI)
find_package(atlas 0.20.2 REQUIRED)

set(MONIO_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MONIO_LIBRARIES monio)

## Sources
add_subdirectory(src)

## Export package
ecbuild_install_project(NAME ${PROJECT_NAME})
ecbuild_print_summary()
