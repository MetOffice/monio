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

execute_process(COMMAND ${CMAKE_COMMAND} -E copy
                        ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py
                        ${CMAKE_BINARY_DIR}/bin/monio_cpplint.py)

ecbuild_add_resources(TARGET       monio_test_scripts
                      SOURCES_PACK ${monio_test_input}
                                   ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py)

if(NOT DEFINED MONIO_TESTFILES_DIR)
  if(DEFINED ENV{MONIO_TESTFILES_DIR})
    set(MONIO_TESTFILES_DIR "$ENV{MONIO_TESTFILES_DIR}")
  elseif(DEFINED ENV{LFRICLITE_DATA_REP_DIR})  # back compat
    set(MONIO_TESTFILES_DIR "$ENV{LFRICLITE_DATA_REP_DIR}")
  elseif(DEFINED MOBB_DATA)
    set(MONIO_TESTFILES_DIR "${MOBB_DATA}/monio")
  elseif(DEFINED ENV{MOBB_DATA})
    set(MONIO_TESTFILES_DIR "$ENV{MOBB_DATA}/monio")
  endif()
endif()
if(IS_DIRECTORY "${MONIO_TESTFILES_DIR}/Data")
  set(MONIO_TESTFILES_DIR "${MONIO_TESTFILES_DIR}/Data")
endif()
if(IS_DIRECTORY "${MONIO_TESTFILES_DIR}")
  message(STATUS "MONIO_TESTFILES_DIR=${MONIO_TESTFILES_DIR}")
  if(IS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Data")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/Data")
  endif()
  file(
    CREATE_LINK
    "${MONIO_TESTFILES_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/Data"
    SYMBOLIC
  )
else()
  message(WARNING "MONIO_TESTFILES_DIR=${MONIO_TESTFILES_DIR}: no such directory")
  return()
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/DataOut)
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
