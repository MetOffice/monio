/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "../monio/StateFull.h"
#include "oops/runs/Run.h"

/// \brief This test targets Monio::readState and Monio::readIncrements. As part of the test it also
///        calls Monio::writeState which has huge overlap with Monio::writeIncrements. This is why
///        it's considered to be a 'full' test. It attempts to recreate an operational environment
///        by creating two cubed-sphere field sets, reading an input file, populating one field set,
///        writing these to file in LFRic order, reading that back in, populating a second field set
///        and comparing them. A test pass is achieved if the entire workflow completes without
///        issue and if the contents of two field sets match at the end of the test.
int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  monio::test::StateFull tests;
  return run.execute(tests);
}
