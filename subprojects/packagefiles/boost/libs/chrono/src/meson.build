# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

api_link_with += library(
  'boost_chrono',
  'chrono.cpp',
  'thread_clock.cpp',
  'process_cpu_clocks.cpp',
  include_directories: api_include_dir,
  dependencies: [threads_dep, rt_dep],
  cpp_args: ['-DBOOST_CHRONO_STATIC_LINK=1'],
  soversion: meson.project_version(),
  install: true,
)
api_compile_args += ['-DBOOST_CHRONO_NO_LIB=1', '-DBOOST_CHRONO_STATIC_LINK=1']
api_dependencies += [threads_dep, rt_dep]
