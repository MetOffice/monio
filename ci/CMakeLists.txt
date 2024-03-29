###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023, Met Office. All rights reserved.                  #
#                                                                             #
# This software is licensed under the terms of the 3-Clause BSD License       #
# which can be obtained from https://opensource.org/license/bsd-3-clause/.    #
###############################################################################

cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

project( monio-ci VERSION 0.0.1 LANGUAGES C CXX Fortran )

set( ENABLE_MPI ON CACHE BOOL "Compile with MPI" )
set( ENABLE_OMP ON CACHE BOOL "Compile with OpenMP" )

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/jedicmake")
if(NOT DEFINED jedicmake_DIR)
  set(jedicmake_DIR "${CMAKE_BINARY_DIR}/jedicmake")
endif()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/oops")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/monio")

include(CTest)
enable_testing()
