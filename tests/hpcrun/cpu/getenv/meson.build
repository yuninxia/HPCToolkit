# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Find the dependency for libdl (-ldl).
dl_dep = dependency('dl', required: true)

# Build the shared library 'libfoo.so' from 'libfoo.c'.
foo_lib = shared_library('foo', 'libfoo.c')

# Build the executable 'getenv' from 'getenv.c'.
getenv_exe = executable(
  'getenv',
  'getenv.c',
  link_with: foo_lib,
  # Link with libdl.
  dependencies: dl_dep,
)

# Test that application getenv is called appropriately
# with and without hpcrun

# Test: Verify that getenv executes successfully
test(
  '@0@ executes successfully'.format(getenv_exe.name()),
  find_program(files('tst-getenv-runs')),
  args: [hpctesttool, getenv_exe],
  suite: 'hpcrun',
  depends: [getenv_exe, foo_lib],
  timeout: 0,  # Signal delivery is very slow on some systems
)

# Test: Measure getenv using hpcrun to verify that even with hpcrun
# present, the application getenv is called when expected
test(
  'measure @0@ with CPUTIME using --enable-auditor'.format(getenv_exe.name()),
  find_program(files('tst-getenv-produces-profiles')),
  args: [hpctesttool, hpcrun, getenv_exe],
  suite: 'hpcrun',
  depends: [getenv_exe, foo_lib, hpcrun_test_depends],
  timeout: 0,  # Signal delivery is very slow on some systems
)
