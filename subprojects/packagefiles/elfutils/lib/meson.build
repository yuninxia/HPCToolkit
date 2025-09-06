# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

libeu_lib = static_library(
  'eu',
  'xasprintf.c',
  'xstrdup.c',
  'xstrndup.c',
  'xmalloc.c',
  'next_prime.c',
  'crc32.c',
  'crc32_file.c',
  'color.c',
  'error.c',
  'printversion.c',
  pic: true,
  include_directories: include_directories('../libelf'),
)
