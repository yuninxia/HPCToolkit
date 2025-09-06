# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

libcpu_lib = static_library(
  'cpu',
  'i386_disasm.c',
  'x86_64_disasm.c',
  'bpf_disasm.c',
  'riscv_disasm.c',
  pic: true,
  include_directories: include_directories(
    '../libelf',
    '../libebl',
    '../libdw',
    '../libasm',
  ),
)
