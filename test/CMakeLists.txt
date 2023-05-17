file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)

list(APPEND lfriclite_testinput
  testinput/fieldset_write.yaml
  testinput/netcdf_write.yaml
  testinput/increment_write.yaml
)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy
                        ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py
                        ${CMAKE_BINARY_DIR}/bin/monio_cpplint.py)

ecbuild_add_resources(TARGET       monio_test_scripts
                      SOURCES_PACK ${monio_test_input}
                                   ${PROJECT_SOURCE_DIR}/test/utils/cpplint.py)
