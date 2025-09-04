# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

subdir('elfutils')

libdwelf_lib = static_library(
  'dwelf',
  'dwelf_elf_gnu_debuglink.c',
  'dwelf_dwarf_gnu_debugaltlink.c',
  'dwelf_elf_gnu_build_id.c',
  'dwelf_scn_gnu_compressed_size.c',
  'dwelf_strtab.c',
  'dwelf_elf_begin.c',
  'dwelf_elf_e_machine_string.c',
  pic: true,
  include_directories: include_directories(
    '../libelf',
    '../libdw',
    '../libdwfl',
    '../libebl',
  ),
)
libdwelf_headers = _eu_headers
