&copy; Crown Copyright Met Office 2023

[![CI](https://github.com/MetOffice/monio/actions/workflows/ci.yml/badge.svg)](https://github.com/MetOffice/monio/actions/workflows/ci.yml)


# Met Office NetCDF Input Output (MONIO)
MONIO has been written in C++17 to handle file I/O for our JEDI model interfaces, but it is also developed for general applicability.

## Description

MONIO represents an extendible set of classes to make reading and writing Network Common Data Form (NetCDF) files. It uses and depends upon Unidata's NetCDF-C++ (https://github.com/Unidata/netcdf-cxx4) library. This carries its own dependencies on Unidata's NetCDF-C (https://github.com/Unidata/netcdf-c) and the HDF Group's HDF5 (https://github.com/HDFGroup/hdf5) libraries, as well as some other supporting libraries. In addition, the primary use-case of MONIO is in the context of the Met Office's (MO) current work with JCSDA's JEDI (https://jointcenterforsatellitedataassimilation-jedi-docs.readthedocs-hosted.com/) framework. For this reason, MONIO also carries a dependency on ECMWF's Atlas (https://github.com/ecmwf/atlas) libraries. However, MONIO is written in an attempt to compartmentalise this dependency. This puts users in a position to take a subset of the source files to construct a NetCDF I/O routine for a novel use-case that be used without it. 

The exception to this is MONIO's current dependency on JCSDA's OOPS (https://github.com/JCSDA/oops). MONIO makes frequent use of OOPS's `oops::Log` tool and its `util::DateTime` 

### Test sub-section

This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. 

#### Sub-sub-section?

This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. This some text. 