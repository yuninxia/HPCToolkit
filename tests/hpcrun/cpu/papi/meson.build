# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if not papi_dep.found()
  subdir_done()
endif

papi_check = executable('papi_check', 'papi_check.c', dependencies: [papi_dep])

test(
  'PAPI profile exist',
  find_program(files('tst-papi-exist')),
  args: [papi_check, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
