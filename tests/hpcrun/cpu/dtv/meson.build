# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Get the C compiler to find necessary libraries.
cc = meson.get_compiler('c')

# Find system libraries required by dtvstress.c, equivalent to -ldl and -lpthread.
dl_dep = cc.find_library('dl', required: true)
thread_dep = dependency('threads', required: true)


# --- Library Generation ---

# Build multiple shared libraries from tlslib.c with different preprocessor macros.

# Define the numeric suffixes for each library type.
dep_nums = [
  '01',
  '02',
  '03',
  '04',
  '05',
  '06',
  '07',
  '08',
  '09',
  '10',
  '11',
  '12',
  '13',
]
dlopen_nums = [
  '01',
  '02',
  '03',
  '04',
  '05',
  '06',
  '07',
  '08',
  '09',
  '10',
  '11',
  '12',
  '13',
  '14',
  '15',
]

# Array to hold the library objects that will be linked against the test executable.
dep_libs = []

# Loop to generate 'deptls' libraries (for direct linking).
foreach num : dep_nums
  lib_name = 'deptls' + num
  instance_def = '-DINSTANCE=' + num
  dep_libs += shared_library(
    lib_name,
    'tlslib.c',
    c_args: [instance_def, '-DLIBTYPE=dep'],
  )
endforeach

# Array to hold library objects needed as runtime dependencies for tests.
dlopen_libs = []

# Loop to generate 'dlopentls' libraries (for dlopen() at runtime).
foreach num : dlopen_nums
  lib_name = 'dlopentls' + num
  instance_def = '-DINSTANCE=' + num
  dlopen_libs += shared_library(
    lib_name,
    'tlslib.c',
    c_args: [instance_def, '-DLIBTYPE=dlopen'],
  )
endforeach

# Build a trivial auditor library.
audit_lib = shared_library('audit', 'trivial-auditor.c')


# --- Executable and Test Setup ---

# Build the dtvstress executable.
test_exe = executable(
  'dtvstress',
  'dtvstress.c',
  dependencies: [dl_dep, thread_dep],
  link_with: dep_libs,
  # This sets the rpath, equivalent to `-Wl,-rpath,`pwd``.
  # It allows the executable to find the linked 'dep' libraries at runtime.
  install_rpath: meson.project_build_root(),
)

# Define the tests to be executed by 'meson test'

# Test: Verify that dtvstress executes successfully
test(
  '@0@ executes successfully'.format(test_exe.name()),
  find_program(files('tst-dtvstress-runs')),
  args: [hpctesttool, test_exe],
  suite: 'hpcrun',
  depends: dlopen_libs,
  timeout: 0,  # Signal delivery is very slow on some systems
)

# Test: Verify that dtvstress fails with an auditor
test(
  '@0@ with an auditor fails'.format(test_exe.name()),
  find_program(files('tst-dtvstress-auditor-fails')),
  args: [hpctesttool, audit_lib, test_exe],
  suite: 'hpcrun',
  depends: dlopen_libs + [audit_lib],
  should_fail: true,
  timeout: 0,  # Signal delivery is very slow on some systems
)

# Test: Measure dlstress using hpcrun to verify that hpcrun doesn't trigger
# memory corruption because of glibc DTV bug
test(
  'measure @0@ with CPUTIME using --enable-auditor'.format(test_exe.name()),
  find_program(files('tst-dtv-realloc-audit-produces-profiles')),
  args: [hpctesttool, hpcrun, test_exe],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  timeout: 0,  # Signal delivery is very slow on some systems
)
