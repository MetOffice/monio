###############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
###############################################################################

set (test_files
     IncrementWrite.h
     NetCDFWrite.h)

list (PREPEND _test_files  "test"  ${test_files} )

set (test_files
     ${_test_files}
     PARENT_SCOPE)
