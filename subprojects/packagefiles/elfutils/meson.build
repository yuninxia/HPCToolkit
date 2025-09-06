# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project(
  'elfutils',
  'c',
  'cpp',
  version: '0.190',
  meson_version: '>=1.1.0',
  default_options: ['c_std=gnu11', 'cpp_std=gnu++11', 'b_staticpic=true'],
)

debuginfod_sover = 1

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

cfg = configuration_data()
cfg.set_quoted('PACKAGE', meson.project_name())
cfg.set_quoted('PACKAGE_NAME', meson.project_name())
cfg.set_quoted(
  'PACKAGE_STRING',
  '@0@ @1@'.format(meson.project_name(), meson.project_version()),
)
cfg.set_quoted('PACKAGE_VERSION', meson.project_version())
cfg.set_quoted('PACKAGE_URL', 'https://sourceware.org/elfutils/')

cfg.set('HAVE_STDINT_H', cc.has_header('stdint.h') ? 1 : false)
cfg.set('HAVE_STDIO_H', cc.has_header('stdio.h') ? 1 : false)
cfg.set('HAVE_STDLIB_H', cc.has_header('stdlib.h') ? 1 : false)
cfg.set('HAVE_STRINGS_H', cc.has_header('strings.h') ? 1 : false)
cfg.set('HAVE_STRING_H', cc.has_header('string.h') ? 1 : false)
cfg.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h') ? 1 : false)
cfg.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h') ? 1 : false)
cfg.set(
  'HAVE_SYS_USER_REGS',
  cc.has_header_symbol('sys/user.h', 'struct user_regs_struct') ? 1 : false,
)
cfg.set('HAVE_UNISTD_H', cc.has_header('unistd.h') ? 1 : false)

cfg.set('SIZEOF_LONG', cc.sizeof('long'))

cfg.set('USE_LOCKS', get_option('thread_safety') ? 1 : false)
threads_dep = dependency('threads')
locks_dep = get_option('thread_safety') ? threads_dep : dependency('', required: false)
cfg.set('HAVE_VISIBILITY', cc.has_function_attribute('visibility:hidden') ? 1 : false)
cfg.set(
  'HAVE_GCC_STRUCT',
  cc.compiles(
    '''
struct test { int x; } __attribute__((gcc_struct));
''',
    name: 'attribute gcc_struct',
  ) ? 1 : false,
)

# Add hardening flags
_args = ['-Wl,-z,relro']
add_project_link_arguments(cc.get_supported_link_arguments(_args), language: 'c')
add_project_link_arguments(cpp.get_supported_link_arguments(_args), language: 'cpp')
_args = ['-Wstack-usage=262144']
add_project_arguments(cc.get_supported_arguments(_args), language: 'c')
add_project_arguments(cpp.get_supported_arguments(_args), language: 'cpp')

# Check for __thread support
assert(
  cc.compiles(
    '''
#undef __thread
static __thread int a; int foo (int b) { return a + b; }
''',
    name: '__thread keyword support',
  ),
)

# Check for stdatomic.h (it should be present in all modern compilers)
cfg.set('HAVE_STDATOMIC_H', cc.has_header('stdatomic.h') ? 1 : false)

# TODO: Unsupported options
cfg.set('DEBUGPRED', false)
cfg.set('CHECK_UNDEFINED', false)
cfg.set('USE_VG_ANNOTATIONS', false)

# Dependencies
zlib_dep = dependency('zlib')
cfg.set('USE_ZLIB', zlib_dep.found() ? 1 : false)
bz2_dep = dependency('bzip2', 'BZip2', required: get_option('bz2'))
cfg.set('USE_BZLIB', bz2_dep.found() ? 1 : false)
lzma_dep = dependency('liblzma', required: get_option('xz'))
cfg.set('USE_LZMA', lzma_dep.found() ? 1 : false)
zstd_dep = dependency('libzstd', required: get_option('zstd'))
cfg.set('USE_ZSTD', zstd_dep.found() ? 1 : false)
cfg.set(
  'USE_ZSTD_COMPRESS',
  get_option('zstd_compress').require(
    zstd_dep.found() and zstd_dep.version().version_compare('>=1.4.0'),
  ).allowed(),
)

cfg.set10(
  'HAVE_DECL_MEMRCHR',
  cc.has_header_symbol('string.h', 'memrchr', args: '-D_GNU_SOURCE'),
)
cfg.set10(
  'HAVE_DECL_RAWMEMCHR',
  cc.has_header_symbol('string.h', 'rawmemchr', args: '-D_GNU_SOURCE'),
)
cfg.set10('HAVE_DECL_POWEROF2', cc.has_header_symbol('sys/param.h', 'powerof2'))
cfg.set10(
  'HAVE_DECL_MEMPCPY',
  cc.has_header_symbol('string.h', 'mempcpy', args: '-D_GNU_SOURCE'),
)
cfg.set10(
  'HAVE_DECL_REALLOCARRAY',
  cc.has_header_symbol('stdlib.h', 'reallocarray', args: '-D_GNU_SOURCE'),
)
cfg.set10(
  'HAVE_DECL_STRERROR_R',
  cc.has_header_symbol('string.h', 'strerror_r', args: '-D_GNU_SOURCE'),
)

cfg.set(
  'HAVE_PROCESS_VM_READV',
  cc.has_function('process_vm_readv', prefix: '#include <sys/uio.h>') ? 1 : false,
)
cfg.set(
  'HAVE_MREMAP',
  cc.has_function('mremap', args: '-D_GNU_SOURCE', prefix: '#include <sys/mman.h>') ? 1 : false,
)

cfg.set('HAVE_ERROR_H', cc.has_header('error.h') ? 1 : false)
cfg.set('HAVE_ERR_H', cc.has_header('err.h') ? 1 : false)
cfg.set('HAVE_EXECINFO_H', cc.has_header('execifno.h') ? 1 : false)
cfg.set('HAVE_SCHED_H', cc.has_header('sched.h') ? 1 : false)
cfg.set(
  'HAVE_SCHED_GETAFFINITY',
  cc.has_function(
    'sched_getaffinity',
    args: '-D_GNU_SOURCE',
    prefix: '#include <sched.h>',
  ) ? 1 : false,
)
cfg.set('HAVE_SYS_RESOURCE_H', cc.has_header('sys/resource.h') ? 1 : false)
cfg.set(
  'HAVE_GETRLIMIT',
  cc.has_function('getrlimit', prefix: '#include <sys/resource.h>') ? 1 : false,
)

add_project_arguments('-D_GNU_SOURCE', language: ['c'])
add_project_dependencies(dependency('intl'), language: ['c'])

demanglelib_dep = cc.find_library('stdc++', required: get_option('demangler'))
cfg.set(
  'USE_DEMANGLE',
  get_option('demangler').require(
    cc.has_function('__cxa_demangle', dependencies: demanglelib_dep),
  ).allowed(),
)

argp_dep = cc.find_library('argp', required: false)
if argp_dep.found()
  assert(cc.has_function('argp_parse', dependencies: argp_dep))
endif
fts_dep = cc.find_library('fts', required: false)
if fts_dep.found()
  assert(cc.has_function('fts_close', dependencies: fts_dep))
endif
obstack_dep = cc.find_library('obstack', required: false)
if obstack_dep.found()
  assert(cc.has_function('_obstack_free', dependencies: obstack_dep))
endif

dl_dep = cc.find_library('dl')

configure_file(output: 'config.h', input: 'config.h.meson', configuration: cfg)
add_project_dependencies(
  declare_dependency(
    include_directories: include_directories('.', 'lib'),
    compile_args: '-DHAVE_CONFIG_H',
  ),
  language: ['c', 'cpp'],
)

subdir('lib')
subdir('libelf')
subdir('libcpu')
subdir('backends')
subdir('libebl')
subdir('libdwelf')
subdir('libdwfl')
subdir('libdw')
# subdir('libasm')
# subdir('debuginfod')
# subdir('src')
# subdir('tests')
