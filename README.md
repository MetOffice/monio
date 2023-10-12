&copy; Crown Copyright Met Office 2023

[![CI](https://github.com/MetOffice/monio/actions/workflows/ci.yml/badge.svg)](https://github.com/MetOffice/monio/actions/workflows/ci.yml)


# Met Office NetCDF Input Output (MONIO)

## Description

MONIO is not just an acronym. It is also so-named because it is _single_-threaded code (mon-; one, single), runs on a _single_ PE in a full MPI context, and the main class is accessible via a _singleton_ design pattern. It also happens that there is a lesser-known Star Wars character with the same name (https://starwars.fandom.com/wiki/Monio). :yawn:

MONIO has been written in C++17 to handle file I/O for the Met Office's (MO) JEDI (https://jointcenterforsatellitedataassimilation-jedi-docs.readthedocs-hosted.com/) Data Assimilation (DA) model interfaces, but it is also developed for general applicability. MONIO represents an extendible set of classes to make reading and writing Network Common Data Form (NetCDF) files more friendly than using the bare NetCDF interface provided by Unidata. However, in its current form it provides a `Monio` singleton class specifically for reading and writing state and increment, UGRID-format (http://ugrid-conventions.github.io/ugrid-conventions/) files generated by and to be consumed by the MO's "LFRic" model (https://www.metoffice.gov.uk/research/approach/modelling-systems/lfric).

## How to Install and Run


### Dependencies

MONIO uses and depends upon Unidata's NetCDF-C++ (https://github.com/Unidata/netcdf-cxx4) library. This carries its own dependencies on Unidata's NetCDF-C (https://github.com/Unidata/netcdf-c) and the HDF Group's HDF5 (https://github.com/HDFGroup/hdf5) libraries, as well as some other supporting libraries.

Due to the primary use-case, MONIO also carries a dependency on ECMWF's Atlas (https://github.com/ecmwf/atlas) library. However, MONIO is written in an attempt to compartmentalise this dependency in the `AtlasReader` and `AtlasWriter` classes. This puts users in a position to take a subset of the source files to construct a NetCDF I/O solution for a novel use-case without Atlas. MONIO makes frequent use of OOPS's(https://github.com/JCSDA/oops) `oops::Log` tool and its `util::DateTime` representation. Though there are likely easy workarounds for these, without modification MONIO will need to be compiled with a compatible version of OOPS.

## Issues

Any questions or issues can be reported to philip.underwood@metoffice.gov.uk.

find_package(MPI REQUIRED COMPONENTS CXX)
find_package(HDF5 REQUIRED COMPONENTS)
find_package(NetCDF COMPONENTS CXX)
find_package(eckit 1.16.1 REQUIRED COMPONENTS MPI)