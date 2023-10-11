/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "../monio/FieldSetWrite.h"
#include "oops/runs/Run.h"

/// \brief This test targets Monio::writeFieldSet. An LFRic background file is read, field sets are
///        populated, and written directly to file. No comparison takes place as the data are left
///        in Atlas order. A pass is achieved simply if the function calls complete without issue.
int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  monio::test::FieldSetWrite tests;
  return run.execute(tests);
}
