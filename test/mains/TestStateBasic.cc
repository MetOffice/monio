/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "../monio/StateBasic.h"
#include "oops/runs/Run.h"

/// \brief This test targets a workflow that avoids the Monio singleton, and the use of Atlas. It
///        reads an input file, writes the data to file, reads that back in and checks the data for
///        integrity. A test pass is achieved if the data first read matches that written.
int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  monio::test::StateBasic tests;
  return run.execute(tests);
}
