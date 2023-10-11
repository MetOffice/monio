###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023, Met Office. All rights reserved.                  #
#                                                                             #
# This software is licensed under the terms of the 3-Clause BSD License       #
# which can be obtained from https://opensource.org/license/bsd-3-clause/.    #
###############################################################################

message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DataOut )

# Currently repurpose data environment variable from LFRic-Lite
set(LFRICLITE_DATA_REP_DIR "$ENV{LFRICLITE_DATA_REP_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                        ${LFRICLITE_DATA_REP_DIR}
                        ${CMAKE_CURRENT_BINARY_DIR}/Data)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)
list(APPEND monio_testinput
  testinput/fieldset_write.yaml
  testinput/state_basic.yaml
  testinput/state_full.yaml
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

ecbuild_add_test(TARGET  test_monio_fieldset_write
                 SOURCES mains/TestFieldSetWrite.cc
                 ARGS    "testinput/fieldset_write.yaml"
                 LIBS    monio
                 MPI     4)

ecbuild_add_test(TARGET  test_monio_state_basic
                 SOURCES mains/TestStateBasic.cc
                 ARGS    "testinput/state_basic.yaml"
                 LIBS    monio
                 MPI     4)

ecbuild_add_test(TARGET  test_monio_state_full
                 SOURCES mains/TestStateFull.cc
                 ARGS    "testinput/state_full.yaml"
                 LIBS    monio
                 MPI     4)

