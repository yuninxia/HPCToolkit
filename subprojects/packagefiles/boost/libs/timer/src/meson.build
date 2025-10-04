# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

api_link_with += library(
  'boost_timer',
  'auto_timers_construction.cpp',
  'cpu_timer.cpp',
  include_directories: api_include_dir,
  cpp_args: ['-DBOOST_TIMER_STATIC_LINK=1'],
  soversion: meson.project_version(),
  install: true,
)
api_compile_args += ['-DBOOST_TIMER_STATIC_LINK=1', '-DBOOST_TIMER_NO_LIB=1']
