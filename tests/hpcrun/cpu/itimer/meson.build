# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

test(
  'CPUTIME event measures on @0@'.format(simple_tstexe.name()),
  find_program(files('tst-cputime-produces-profiles')),
  args: [hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'CPUTIME event measures on @0@ w/o auditor'.format(simple_tstexe.name()),
  find_program(files('tst-cputime-produces-profiles-noaudit')),
  args: [hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'REALTIME event measures on @0@'.format(simple_tstexe.name()),
  find_program(files('tst-realtime-produces-profiles')),
  args: [hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'REALTIME event measures on @0@ w/o auditor'.format(simple_tstexe.name()),
  find_program(files('tst-realtime-produces-profiles-noaudit')),
  args: [hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
