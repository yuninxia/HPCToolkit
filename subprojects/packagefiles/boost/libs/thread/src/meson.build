# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

boost_thread_args = ['-DBOOST_THREAD_BUILD_LIB=1']

if host_machine.system() in ['linux']
  boost_thread_srcs = files('future.cpp', 'pthread/once.cpp', 'pthread/thread.cpp')
  boost_thread_args += ['-DBOOST_THREAD_POSIX', '-DBOOST_THREAD_DONT_USE_CHRONO']
else
  error(
    'Host system unsupported by Boost::Thread wrap: @0@'.format(host_machine.system()),
  )
endif

api_link_with += library(
  'boost_thread',
  boost_thread_srcs,
  include_directories: api_include_dir,
  dependencies: [threads_dep],
  cpp_args: boost_thread_args,
  soversion: meson.project_version(),
  install: true,
)
api_compile_args += ['-DBOOST_THREAD_BUILD_LIB=1', '-DBOOST_THREAD_NO_LIB=1']
api_dependencies += [threads_dep]
