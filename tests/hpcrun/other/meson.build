# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

tstexe = executable(
  'tstexe-pthread-key-canary',
  'pthread-key-canary.c',
  dependencies: [threads_dep, dl_dep],
  link_with: shared_library('tstexe-pthread-key-helper', 'pthread-key-helper.c'),
)
test(
  'Hpcrun does not taint pthread_keys',
  find_program(files('tst-runs')),
  args: [hpcrun, tstexe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
)
test(
  'Hpcrun does not taint pthread_keys w/o auditor',
  find_program(files('tst-runs')),
  args: [hpcrun, tstexe, '--disable-auditor'],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
)
