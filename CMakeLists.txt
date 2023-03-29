cmake_minimum_required(VERSION 3.12)

project(monio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake
set(CMAKE_DIRECTORY_LABELS ${PROJECT_NAME})

find_package(eckit REQUIRED)
find_package(atlas REQUIRED)
find_package(NetCDF REQUIRED COMPONENTS C)

add_subdirectory( src )

ecbuild_install_project( NAME ${PROJECT_NAME} )
