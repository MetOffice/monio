###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2024, Met Office. All rights reserved.                  #
#                                                                             #
# This software is licensed under the terms of the 3-Clause BSD License       #
# which can be obtained from https://opensource.org/license/bsd-3-clause/.    #
###############################################################################

cmake_minimum_required( VERSION 3.21 FATAL_ERROR )
project( monio-ci VERSION 0.0.1 LANGUAGES C CXX Fortran )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )
set( ENABLE_MPI ON CACHE BOOL "Compile with MPI" )
set( ENABLE_OMP ON CACHE BOOL "Compile with OpenMP" )

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/jedicmake" EXCLUDE_FROM_ALL)
if(NOT DEFINED jedicmake_DIR)
  set(jedicmake_DIR "${CMAKE_BINARY_DIR}/jedicmake")
endif()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/oops" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/monio")

enable_testing()
