# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project('boost', 'cpp', version: '1.89.0', meson_version: '>=1.2.0')

fs = import('fs')
cpp = meson.get_compiler('cpp')

# Confirm the version is up-to-date
foreach line : fs.read('Jamroot').splitlines()
  if line.strip().startswith('constant BOOST_VERSION')
    boost_version = line.split(':')[1].strip().strip(';').strip()
  endif
endforeach
assert(boost_version == meson.project_version(), 'Boost wrap version is out-of-date!')

rt_dep = cpp.find_library('rt')
threads_dep = dependency('threads')

# Boost is organized into separate libraries, but for the wrap we mash it all together.
# These shared variables collect the fragments from each library.
api_include_dir = include_directories('.')
api_link_with = []
api_compile_args = []
api_dependencies = []

subdir('libs/atomic/src')
subdir('libs/chrono/src')
subdir('libs/graph/src')
subdir('libs/filesystem/src')
subdir('libs/thread/src')
subdir('libs/timer/src')

# Final dependency for the parent project to use
boost_dep = declare_dependency(
  include_directories: api_include_dir,
  link_with: api_link_with,
  compile_args: api_compile_args,
  dependencies: api_dependencies,
)
meson.override_dependency('boost', boost_dep)
