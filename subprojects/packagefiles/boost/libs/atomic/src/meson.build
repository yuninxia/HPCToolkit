# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

boost_atomic_args = ['-DBOOST_ATOMIC_SOURCE', '-DBOOST_ATOMIC_STATIC_LINK=1']

if cpp.has_header_symbol('pthread.h', 'pthread_cond_clockwait')
  boost_atomic_args += ['-DBOOST_ATOMIC_HAS_PTHREAD_COND_CLOCKWAIT']
endif

api_link_with += library(
  'boost_atomic',
  'lock_pool.cpp',
  include_directories: api_include_dir,
  cpp_args: boost_atomic_args,
  soversion: meson.project_version(),
  install: true,
)
api_compile_args += ['-DBOOST_ATOMIC_NO_LIB=1', '-DBOOST_ATOMIC_STATIC_LINK=1']
