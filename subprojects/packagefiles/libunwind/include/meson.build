# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

configure_file(output: 'config.h', configuration: cfg)

ver = meson.project_version().split('.')
libunwind_common = configure_file(
  output: 'libunwind-common.h',
  input: 'libunwind-common.h.in',
  configuration: {'PKG_MAJOR': ver[0], 'PKG_MINOR': ver[1], 'PKG_EXTRA': ver[2]},
)

_headers = [libunwind_common] + files(
  'libunwind-aarch64.h',
  'libunwind-arm.h',
  'libunwind-coredump.h',
  'libunwind-dynamic.h',
  'libunwind-hppa.h',
  'libunwind-ia64.h',
  'libunwind-loongarch64.h',
  'libunwind-mips.h',
  'libunwind-nto.h',
  'libunwind-ppc32.h',
  'libunwind-ppc64.h',
  'libunwind-ptrace.h',
  'libunwind-riscv.h',
  'libunwind-s390x.h',
  'libunwind-sh.h',
  'libunwind-x86.h',
  'libunwind-x86_64.h',
  'libunwind.h',
  'unwind.h',
)

subdir('api')

internal_inc = [include_directories('.'), iface_inc]
