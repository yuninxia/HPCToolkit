# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if not python.found()
  subdir_done()
endif

simple_tstexe = executable(
  'tstexe-simple-spin',
  '../cpu/simple-spin.c',
  dependencies: [math_dep],
)

_tst = find_program(files('tst-python-simple'))
test(
  'Measurement of simple-1thread unwound Python',
  _tst,
  args: [hpctesttool, hpcrun, hpcprof, python.full_path(), files('simple-1thread'), '1'],
  suite: ['hpcrun', 'python'],
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'Measurement of simple-mthread unwound Python',
  _tst,
  args: [hpctesttool, hpcrun, hpcprof, python.full_path(), files('simple-mthread'), '3'],
  suite: ['hpcrun', 'python'],
  depends: hpcrun_test_depends,
  # FIXME: The Python support currently can't detect Python-spawned threads
  should_fail: true,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'Measurement of simple-exception unwound Python',
  _tst,
  args: [
    hpctesttool,
    hpcrun,
    hpcprof,
    python.full_path(),
    files('simple-exception'),
    '1',
  ],
  suite: ['hpcrun', 'python'],
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'Measurement of simple-signals unwound Python',
  _tst,
  args: [hpctesttool, hpcrun, hpcprof, python.full_path(), files('simple-signals'), '1'],
  suite: ['hpcrun', 'python'],
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

_tst = find_program(files('tst-python-simple-noaudit'))
test(
  'Measurement of simple-1thread unwound Python w/o auditor',
  _tst,
  args: [hpctesttool, hpcrun, hpcprof, python.full_path(), files('simple-1thread'), '1'],
  suite: ['hpcrun', 'python'],
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'Measurement of (non-python) @0@ using "-a python" w/ auditor'.format(
    simple_tstexe.name(),
  ),
  find_program(files('tst-simple-produces-profiles')),
  args: [hpctesttool, hpcrun, simple_tstexe],
  suite: ['hpcrun', 'python'],
  depends: [hpcrun_test_depends],
  timeout: 0,  # Signal delivery is very slow on some systems
)
