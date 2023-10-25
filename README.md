&copy; Crown Copyright Met Office 2023

[![CI](https://github.com/MetOffice/monio/actions/workflows/ci.yml/badge.svg)](https://github.com/MetOffice/monio/actions/workflows/ci.yml)


# Met Office NetCDF Input Output (MONIO)

## Description

MONIO is not just an acronym. It is also so-named because it is _single_-threaded code (_mon-_; one, single), runs on a _single_ PE in a full MPI context, and the main class is accessible via a _singleton_ design pattern. It also happens that there is a lesser-known Star Wars character with the same name (https://starwars.fandom.com/wiki/Monio) :neutral_face:

MONIO has been written in C++17 to handle file I/O for the Met Office's (MO) JEDI (https://jointcenterforsatellitedataassimilation-jedi-docs.readthedocs-hosted.com/) Data Assimilation (DA) model interfaces, but it is also developed with a gesture toward general applicability. MONIO represents an extendible set of classes to make reading and writing Network Common Data Form (NetCDF) files more friendly than using the bare NetCDF interface provided by Unidata alone.

MONIO provides a `Monio` singleton class specifically for reading and writing state and increment, UGRID-format (http://ugrid-conventions.github.io/ugrid-conventions/) files generated by and to be consumed by the MO's "LFRic" model (https://www.metoffice.gov.uk/research/approach/modelling-systems/lfric). For this reason, it carries a few dependencies that are characteristic of any other JEDI-based repository.

## How to Install and Run

### Dependencies

MONIO is a CMake project. However, it is configured to make use of ECMWF's ECBuild (https://github.com/ecmwf/ecbuild). The `CMakeLists.txt` file will need to modified if a dependency on ECBuild is to be avoided.

At its core, MONIO uses and depends upon Unidata's NetCDF-C++ (https://github.com/Unidata/netcdf-cxx4) library. This carries its own dependencies on Unidata's NetCDF-C (https://github.com/Unidata/netcdf-c) and the HDF Group's HDF5 (https://github.com/HDFGroup/hdf5) libraries, as well as some other supporting libraries.

Having been built for use with JEDI, MONIO also carries a dependency on ECMWF's Atlas (https://github.com/ecmwf/atlas) library. However, MONIO is written to compartmentalise this dependency in the `AtlasReader` and `AtlasWriter` classes. This puts users in a position to drop these classes to construct a NetCDF I/O solution without Atlas.

MONIO makes frequent use of JCSDA's OOPS (https://github.com/JCSDA/oops) `oops::Log` tool and its `util::DateTime` representation. Some light modifications could avoid these, but without modification MONIO will need to be compiled with a compatible version of OOPS.

As it is intended to run within an operational, meteorological DA context, MONIO is written to function with Message Passing Interface (MPI) communicators. JEDI wraps these with C++ code provided by ECMWF's ECKit (https://github.com/ecmwf/eckit), and MONIO adopts this approach. For this reason, MONIO also carries a dependency on ECKit. Which itself carries an underlying dependency on a suitable MPI implementation, e.g., Open MPI (https://www.open-mpi.org/software/).

### Compilation

After satisfying the dependencies outlined above, MONIO was built and tested using the Make and Ninja build systems, and the GCC and Intel compilers.

## How to Use

MONIO has been written to address the known use cases defined within the MO-JEDI context. These are captured in the public functons defined in the `Monio` singleton class. Once `Monio.h` is included in a source file it can be used directly. 

In the following scenarios, the only common parameter is an `atlas::FieldSet` that uses an `atlas::CubedSphereGrid` and an `atlas::Mesh` configured as a `"cubedsphere_dual"`. Passing an `atlas::FieldSet` of a different configuration can prevent MONIO from functioning correctly.

Most of the functions in `monio::Monio` also take a reference to a `std::vector<consts::FieldMetadata>`. This is a standard C++ `std::vector` of `consts::FieldMetadata` structs defined in MONIO's `Constants.h` file. The struct instances in this vector must correspond to an `atlas::Field` in the `atlas::FieldSet`. They are accessed by the value of `FieldMetadata.jediName`. If an `atlas::Field` in the `atlas::FieldSet` has no corresponding instance of `consts::FieldMetadata`, they will not be processed. This may be an intended use-case. However, if an instance of `consts::FieldMetadata` does not correspond to an `atlas::Field` in the `atlas::FieldSet`, an exeption will be generated and program execution will cease.

### Reading State Files

Reading of an LFRic-compatible, time-dependent, background file can be carried out with the following call:

```
monio::Monio::get().readState(localFieldSet, fieldMetadataVec, filePath, dateTime);
```

Where `localFieldSet` is the `atlas::FieldSet` to be populated with data from the file, `fieldMetadataVec` is the `std::vector<consts::FieldMetadata>`, `filePath` is a `std::string` defining a valid path to the file to be read, and `dateTime` is an instance of `util::DateTime` indicating what position in the time series data are required for.

### Reading Increment Files

Reading of an LFRic-compatible, time-independent, increment file can be carried out with the following call:

```
monio::Monio::get().readIncrements(localFieldSet, fieldMetadataVec, filePath);
```

Where `localFieldSet` is the `atlas::FieldSet` to be populated with data from the file, `fieldMetadataVec` is the `std::vector<consts::FieldMetadata>`, and `filePath` is a `std::string` defining a valid path to the file to be read.

### Writing Increment Files

Writing of an LFRic-compatible, time-independent, increment file is dependent on geometry and other metadata being available at the resolution you intend to write. These will be availalbe if MONIO has already been used to read LFRic-compatible data at the same resolution you intend to write (see the functions described above). If MONIO has not been used for reading, writing will first require that geometry and metadata are copied from an appropriate input file using the following call:

```
monio::Monio::get().initialiseFile(grid, filePath); 
```

Where `grid` is a reference to the `atlas::CubedSphereGrid` used to setup the geometry of the `atlas::FieldSet`, `filePath` is a `std::string` defining a valid path to the file to be read. This particular function call returns an `int` that determines the variable naming convention used in the input file. However, it is necessary to capture and use that `int` here. 

Following this, writing of an LFRic-compatible, time-independent, increment file can be carried out with the following call:

```
monio::Monio::get().writeIncrements(localFieldSet, fieldMetadataVec, filePath, isLFRicNaming);
```

Where `localFieldSet` is the `atlas::FieldSet` containing the data to be written to file, `fieldMetadataVec` is the `std::vector<consts::FieldMetadata>`, `filePath` is a `std::string` defining a valid path to the intended output file, and optionally, `isLFRicNaming` is a `bool` defining whether or not the variables should use LFRic or JEDI names. If this parameter is not defined, the variables will take the corresponding `FieldMetadata.lfricWriteName` by default. The LFRic name is the only difference this function has with `Monio::writeState` (below).

### Writing State Files

_This method is intended for use with tests only_. Writing of an LFRic-compatible, time-independent, state file is dependent on geometry and other metadata being available at the resolution you intend to write. These will be availalbe if MONIO has already been used to read LFRic-compatible data at the same resolution you intend to write (see the functions described above). If MONIO has not been used for reading, writing will first require that geometry and metadata are copied from an appropriate input file using the following call:

```
monio::Monio::get().initialiseFile(grid, filePath); 
```

Where `grid` is a reference to the `atlas::CubedSphereGrid` used to setup the geometry of the `atlas::FieldSet`, `filePath` is a `std::string` defining a valid path to the file to be read. This particular function call returns an `int` that determines the variable naming convention used in the input file. However, it is necessary to capture and use that `int` here. 

Following this, writing of an LFRic-compatible, time-independent, state file can be carried out with the following call:

```
monio::Monio::get().writeState(localFieldSet, fieldMetadataVec, filePath, isLFRicNaming);
```

Where `localFieldSet` is the `atlas::FieldSet` containing the data to be written to file, `fieldMetadataVec` is the `std::vector<consts::FieldMetadata>`, `filePath` is a `std::string` defining a valid path to the intended output file, and optionally, `isLFRicNaming` is a `bool` defining whether or not the variables should use LFRic or JEDI names. If this parameter is not defined, the variables will take the corresponding `FieldMetadata.lfricReadName` by default. The LFRic name is the only difference this function has with `Monio::writeIncrements` (above).

### Writing A FieldSet

For debugging, it may occasionally be useful to output a `FieldSet` from any arbitrary position in the code into a NetCDF so that it can be examined. For this reason, MONIO offers the following call:

```
monio::Monio::get().writeFieldSet(localFieldSet, filePath);
```

Where `localFieldSet` is the `atlas::FieldSet` containing the data to be written to file, and `filePath` is a `std::string` defining a valid path to the intended output file.

## Issues

Any questions or issues can be raised on https://github.com/MetOffice/monio/issues or reported to philip.underwood@metoffice.gov.uk.
