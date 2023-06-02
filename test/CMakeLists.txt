###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)

list(APPEND monio_testinput
  testinput/netcdf_write.yaml
)

foreach(FILENAME ${monio_testinput})
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
                           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME})
endforeach(FILENAME)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy
                        ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py
                        ${CMAKE_BINARY_DIR}/bin/monio_cpplint.py)

ecbuild_add_resources(TARGET       monio_test_scripts
                      SOURCES_PACK ${monio_test_input}
                                   ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py)

ecbuild_add_test(TARGET monio_coding_norms
                 TYPE SCRIPT
                 COMMAND ${CMAKE_BINARY_DIR}/bin/monio_cpplint.py
                 ARGS --quiet --recursive
                       ${PROJECT_SOURCE_DIR}/src
                       ${PROJECT_SOURCE_DIR}/test)

ecbuild_add_test(TARGET  test_monio_netcdf_write
                 SOURCES mains/TestNetCDFWrite.cc
                 ARGS    "testinput/netcdf_write.yaml"
                 LIBS    monio
                 MPI     4)
