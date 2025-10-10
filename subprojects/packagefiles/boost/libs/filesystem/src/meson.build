# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

boost_filesystem_args = [
  '-DBOOST_FILESYSTEM_SOURCE',
  '-DBOOST_FILESYSTEM_STATIC_LINK=1',
]

if cpp.compiles(
  files('../config/has_attribute_init_priority.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_INIT_PRIORITY']
endif
if cpp.compiles(
  files('../config/has_stat_st_mtim.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_BLKSIZE']
endif
if cpp.compiles(
  files('../config/has_stat_st_mtim.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_MTIM']
endif
if cpp.compiles(
  files('../config/has_stat_st_mtimensec.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_MTIMENSEC']
endif
if cpp.compiles(
  files('../config/has_stat_st_mtimespec.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_MTIMESPEC']
endif
if cpp.compiles(
  files('../config/has_stat_st_birthtim.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_BIRTHTIM']
endif
if cpp.compiles(
  files('../config/has_stat_st_birthtimensec.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_BIRTHTIMENSEC']
endif
if cpp.compiles(
  files('../config/has_stat_st_birthtimespec.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STAT_ST_BIRTHTIMESPEC']
endif
if cpp.compiles(
  files('../config/has_fdopendir_nofollow.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_FDOPENDIR_NOFOLLOW']
endif
if cpp.compiles(
  files('../config/has_dirent_d_type.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_DIRENT_D_TYPE']
endif
if cpp.compiles(
  files('../config/has_posix_at_apis.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_POSIX_AT_APIS']
endif
if cpp.compiles(
  files('../config/has_fallocate.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_FALLOCATE']
endif
if cpp.compiles(
  files('../config/has_statx.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STATX']
elif cpp.compiles(
  files('../config/has_statx_syscall.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_HAS_STATX_SYSCALL']
endif
if not cpp.compiles(
  files('../config/has_cxx20_atomic_ref.cpp'),
  include_directories: include_directories('.'),
)
  boost_filesystem_args += ['-DBOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF']
endif

api_link_with += library(
  'boost_filesystem',
  'codecvt_error_category.cpp',
  'exception.cpp',
  'directory.cpp',
  'operations.cpp',
  'path.cpp',
  'path_traits.cpp',
  'portability.cpp',
  'unique_path.cpp',
  'utf8_codecvt_facet.cpp',
  include_directories: api_include_dir,
  cpp_args: boost_filesystem_args,
  soversion: meson.project_version(),
  install: true,
)
api_compile_args += ['-DBOOST_FILESYSTEM_STATIC_LINK=1', '-DBOOST_FILESYSTEM_NO_LIB=1']
