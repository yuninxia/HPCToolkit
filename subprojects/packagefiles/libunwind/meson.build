# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project(
  'libunwind',
  'c',
  'cpp',
  version: '1.8.0',
  meson_version: '>=1.1.0',
  default_options: ['b_ndebug=if-release'],
)

cc = meson.get_compiler('c')

cfg = configuration_data()

cfg.set_quoted('PACKAGE', meson.project_name())
cfg.set_quoted('PACKAGE_NAME', meson.project_name())
cfg.set_quoted('PACKAGE_TARNAME', meson.project_name())
cfg.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/libunwind/libunwind/issues')
cfg.set_quoted('PACKAGE_URL', 'https://github.com/libunwind/libunwind')
cfg.set_quoted(
  'PACKAGE_STRING',
  '@0@ @1@'.format(meson.project_name(), meson.project_version()),
)
cfg.set_quoted('PACKAGE_VERSION', meson.project_version())

cfg.set('SIZEOF_OFF_T', cc.sizeof('off_t', prefix: '#include <sys/types.h>'))

cfg.set('HAVE_ASM_PTRACE_OFFSETS_H', cc.has_header('asm/ptrace_offsets.h') ? 1 : false)
cfg.set('HAVE_ASM_PTRACE_H', cc.has_header('asm/ptrace.h') ? 1 : false)
cfg.set('HAVE_ASM_VSYSCALL_H', cc.has_header('asm/vsyscall.h') ? 1 : false)
cfg.set('HAVE_ENDIAN_H', cc.has_header('endian.h') ? 1 : false)
cfg.set('HAVE_SYS_ENDIAN_H', cc.has_header('sys/endian.h') ? 1 : false)
cfg.set('HAVE_SYS_PARAM_H', cc.has_header('sys/param.h') ? 1 : false)
cfg.set('HAVE_EXECINFO_H', cc.has_header('execinfo.h') ? 1 : false)
cfg.set('HAVE_IA64INTRIN_H', cc.has_header('ia64intrin.h') ? 1 : false)
cfg.set('HAVE_SYS_UC_ACCESS_H', cc.has_header('sys/uc_access.h') ? 1 : false)
cfg.set('HAVE_UNISTD_H', cc.has_header('unistd.h') ? 1 : false)
cfg.set('HAVE_SIGNAL_H', cc.has_header('signal.h') ? 1 : false)
cfg.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h') ? 1 : false)
cfg.set('HAVE_SYS_PROCFS_H', cc.has_header('sys/procfs.h') ? 1 : false)
cfg.set('HAVE_SYS_PTRACE_H', cc.has_header('sys/ptrace.h') ? 1 : false)
cfg.set('HAVE_SYS_SYSCALL_H', cc.has_header('sys/syscall.h') ? 1 : false)
cfg.set('HAVE_BYTESWAP_H', cc.has_header('byteswap.h') ? 1 : false)
cfg.set('HAVE_ELF_H', cc.has_header('elf.h') ? 1 : false)
cfg.set('HAVE_SYS_ELF_H', cc.has_header('sys/elf.h') ? 1 : false)
cfg.set('HAVE_LINK_H', cc.has_header('link.h') ? 1 : false)
cfg.set('HAVE_SYS_LINK_H', cc.has_header('sys/link.h') ? 1 : false)

cfg.set(
  'HAVE_STRUCT_DL_PHDR_INFO_DLPI_SUBS',
  cc.has_member('struct dl_phdr_info', 'dlpi_subs', prefix: '#include <link.h>') ? 1 : false,
)
cfg.set(
  'HAVE_STRUCT_ELF_PRSTATUS',
  cc.has_type('struct elf_prstatus', prefix: '#include <sys/procfs.h>') ? 1 : false,
)
cfg.set(
  'HAVE_STRUCT_PRSTATUS',
  cc.has_type('struct prstatus', prefix: '#include <sys/procfs.h>') ? 1 : false,
)
cfg.set(
  'HAVE_PROCFS_STATUS',
  cc.has_type('procfs_status', prefix: '#include <sys/procfs.h>') ? 1 : false,
)
cfg.set(
  'HAVE_ELF_FPREGSET_T',
  cc.has_type('elf_fpregset_t', prefix: '#include <sys/procfs.h>') ? 1 : false,
)

add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])

libcrts_dep = dependency('', required: false)
if not cc.has_function('_Unwind_Resume')
  libcrts_dep = cc.find_library('gcc_s', required: false)
  if (not cppunwind_dep.found()
or not cc.has_function('_Unwind_Resume', dependencies: cppunwind_dep)
)
    libcrts_dep = cc.find_library('gcc', required: false)
    if (not cppunwind_dep.found()
or not cc.has_function('_Unwind_Resume', dependencies: cppunwind_dep)
)
      libcrts_dep = dependency('', required: false)
    endif
  endif
endif

cfg.set(
  'HAVE_DL_ITERATE_PHDR',
  cc.has_function('dl_iterate_phdr', args: '-D_GNU_SOURCE', prefix: '#include <link.h>') ? 1 : false,
)
cfg.set(
  'HAVE_DL_PHDR_REMOVALS_COUNTER',
  cc.has_function('dl_phdr_removals_counter') ? 1 : false,
)
cfg.set('HAVE_DLMODINFO', cc.has_function('dlmodinfo') ? 1 : false)
cfg.set('HAVE_GETUNWIND', cc.has_function('getunwind') ? 1 : false)
cfg.set('HAVE_TTRACE', cc.has_function('ttrace') ? 1 : false)
cfg.set(
  'HAVE_MINCORE',
  cc.has_function('mincore', prefix: '#include <sys/mman.h>') ? 1 : false,
)
cfg.set(
  'HAVE_PIPE2',
  cc.has_function('pipe2', prefix: '#include <unistd.h>') ? 1 : false,
)
cfg.set(
  'HAVE_SIGALTSTACK',
  cc.has_function('sigaltstack', prefix: '#include <signal.h>') ? 1 : false,
)
cfg.set(
  'HAVE_EXECVPE',
  cc.has_function('execvpe', prefix: '#include <unistd.h>') ? 1 : false,
)

build_coredump = (get_option('coredump').enable_auto_if(
  host_machine.cpu_family() in [
    'aarch64',
    'arm',
    'mips',
    'mips64',
    'sh4',
    'x86',
    'x86_64',
    'riscv32',
    'riscv64',
    'loongarch64',
  ],
).enabled()
)
build_ptrace = (get_option('ptrace').disable_if(
  cfg.get('HAVE_SYS_PTRACE_H') != 1,
  error_message: '-Dptrace=enabled but ptrace not supported on target',
).allowed()
)

if build_ptrace
  cfg.set(
    'HAVE_DECL_PTRACE_POKEUSER',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_POKEUSER') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_POKEDATA',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_POKEDATA') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_SETREGSET',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SETREGSET') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_TRACEME',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_TRACEME') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_CONT',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_CONT') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_SINGLESTEP',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SINGLESTEP') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PTRACE_SYSCALL',
    cc.has_header_symbol('sys/ptrace.h', 'PTRACE_SYSCALL') ? 1 : false,
  )
  cfg.set('HAVE_DECL_PT_IO', cc.has_header_symbol('sys/ptrace.h', 'PT_IO') ? 1 : false)
  cfg.set(
    'HAVE_DECL_PT_GETREGS',
    cc.has_header_symbol('sys/ptrace.h', 'PT_GETREGS') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PT_GETFPREGS',
    cc.has_header_symbol('sys/ptrace.h', 'PT_GETFPREGS') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PT_CONTINUE',
    cc.has_header_symbol('sys/ptrace.h', 'PT_CONTINUE') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PT_TRACE_ME',
    cc.has_header_symbol('sys/ptrace.h', 'PT_TRACE_ME') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PT_STEP',
    cc.has_header_symbol('sys/ptrace.h', 'PT_STEP') ? 1 : false,
  )
  cfg.set(
    'HAVE_DECL_PT_SYSCALL',
    cc.has_header_symbol('sys/ptrace.h', 'PT_SYSCALL') ? 1 : false,
  )
endif

# FIXME
build_nto = false
build_setjmp = false

dl_dep = cc.find_library('dl')
threads_dep = dependency('threads')
execinfo_dep = dependency('', required: false)
if not cc.has_function('backtrace')
  execinfo_dep = cc.find_library('execinfo')
endif

if (get_option('b_ndebug') == 'false'
or (get_option('b_ndebug') == 'if-release'
and get_option('buildtype') == 'release'
)
)
  add_project_arguments('-DDEBUG', language: ['c', 'cpp'])
endif

support_cxx_exceptions = (get_option('cxx_exceptions').enable_auto_if(
  host_machine.cpu_family() in [
    'aarch64',
    'arm',
    'mips',
    'mips64',
    'x86',
    'x86_64',
    's390x',
    'loongarch64',
  ],
).enabled()
)

config_docs = false

if host_machine.cpu_family() != target_machine.cpu_family()
  add_project_arguments('-DUNW_REMOTE_ONLY', language: ['c', 'cpp'])
endif

cfg.set(
  'CONFIG_DEBUG_FRAME',
  get_option('debug_frame').enable_auto_if(
    host_machine.cpu_family() in ['arm', 'aarch64'],
  ).enabled() ? 1 : false,
)
cfg.set('CONFIG_BLOCK_SIGNALS', get_option('block_signals') ? 1 : false)
cfg.set('CONSERVATIVE_CHECKS', get_option('conservative_checks') ? 1 : false)

lzma_dep = dependency('liblzma', required: get_option('minidebuginfo'))
cfg.set('HAVE_LZMA', lzma_dep.found() ? 1 : false)

zlib_dep = dependency('zlib', required: get_option('zlibdebuginfo'))
cfg.set('HAVE_ZLIB', zlib_dep.found() ? 1 : false)

cfg.set(
  'HAVE__BUILTIN___CLEAR_CACHE',
  cc.compiles(
    'int main() { __builtin___clear_cache(0, 0); }',
    name: 'has __builtin___clear_cache',
  ) ? 1 : false,
)
cfg.set(
  'HAVE__BUILTIN_UNREACHABLE',
  cc.compiles(
    'int main() { __builtin_unreachable(); }',
    name: 'has __builtin_unreachable',
  ) ? 1 : false,
)

subdir('include')
subdir('src')
