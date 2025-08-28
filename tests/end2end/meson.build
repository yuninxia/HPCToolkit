# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

_tst = find_program(files('tst-default-workflow'))
_tst_ss = find_program(files('tst-single-structfile-workflow'))
foreach exe : [
  executable('tstexe-e2e-openmp', files('openmp.cpp'), dependencies: openmp_dep),
]
  test(
    'Default analysis workflow on @0@ works'.format(exe.name()),
    _tst,
    args: [hpctesttool, hpcrun, hpcstruct, hpcprof, exe],
    suite: 'end2end',
    env: test_env,
    depends: test_depends,
    timeout: 0,  # Workload can vary widely due to system differences
  )
  test(
    'Single-structfile analysis workflow on @0@ works'.format(exe.name()),
    _tst_ss,
    args: [hpctesttool, hpcrun, hpcstruct, hpcprof, exe],
    suite: 'end2end',
    env: test_env,
    depends: test_depends,
    timeout: 240,  # Multithreaded tests need more time
  )
endforeach
