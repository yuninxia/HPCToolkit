# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Build test utility to check if perf_event_open syscall is functional
perf_check = executable('perf_check', files('perf_check.c'))

test(
  'Perf frequency on group of events',
  find_program(files('tst-perf-freq-group')),
  args: [perf_check, hpcproftt, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'Perf period on group of events',
  find_program(files('tst-perf-period-group')),
  args: [perf_check, hpcproftt, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'Perf period sampling measures on @0@'.format(simple_tstexe.name()),
  find_program(files('tst-perf-period-produces-profiles')),
  args: [perf_check, hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'Perf period sampling measures on @0@ w/o auditor'.format(simple_tstexe.name()),
  find_program(files('tst-perf-period-produces-profiles-noaudit')),
  args: [perf_check, hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)

test(
  'Perf frequency sampling measures on @0@'.format(simple_tstexe.name()),
  find_program(files('tst-perf-freq-produces-profiles')),
  args: [perf_check, hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
test(
  'Perf frequency sampling measures on @0@ w/o auditor'.format(simple_tstexe.name()),
  find_program(files('tst-perf-freq-produces-profiles-noaudit')),
  args: [perf_check, hpctesttool, hpcrun, simple_tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
