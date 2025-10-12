# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

project(
  'hpctoolkit',
  'c',
  'cpp',
  version: '2025.1.0-alpha',
  meson_version: '>=1.6.0',
  license: 'Apache-2.0',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu11',
    'cpp_std=gnu++17',
    'cuda_std=c++14',
    'b_ndebug=if-release',
  ],
)

fs = import('fs')
pymod = import('python')
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')


# FIXME: -Wunused-result reports warnings all over the codebase. Until this can be fixed, prevent
# it from causing complete compilation failure when running with -Werror.
if get_option('werror')
  foreach _lang : ['c', 'cpp']
    add_project_arguments(
      meson.get_compiler(_lang).first_supported_argument('-Wno-error=unused-result'),
      language: _lang,
    )
  endforeach
endif

# Find Python for the build machine. We use Python scripts to perform various tasks
# during the build, so this is a hard requirement.
python_names = ['python3', 'python']
python_versions = ['>=3.8', '<4']
python_versioned_names = []
foreach version : range(8, 20)
  python_versioned_names += [f'python3.@version@']
endforeach
build_python = find_program(
  python_names,
  version: python_versions,
  native: true,
  required: false,
)
if not build_python.found()
  build_python = find_program(
    python_versioned_names,
    version: python_versions,
    native: true,
  )
endif

# Find liblzma. We search for this first so our default_options can apply before it gets
# used in later wraps.
lzma_dep = dependency(
  'liblzma',
  version: '>=5.2.0',
  include_type: 'system',
  default_options: ['warning_level=0'],
)

# Find Elfutils. We search for libdw first because if libelf is installed but libdw isn't, the
# dependency('libdw') call will invoke the Elfutils subproject and call meson.override_dependency()
# for libelf, if we found libelf first this call causes a Meson setup failure.
# In all major distros you can't install libdw without libelf, so this shouldn't cause problems.
# NB: We don't use bzip2, but the Elfutils wrap might, so configure its warnings here.
elfutils_default_options = [
  'thread_safety=true',
  'warning_level=0',
  'werror=false',
  'bzip2:warning_level=0',
  'bzip2:werror=false',
]
libdw_dep = dependency(
  'libdw',
  version: '>=0.186',
  default_options: elfutils_default_options,
  include_type: 'system',
)
libelf_dep = dependency(
  'libelf',
  version: '>=0.186',
  default_options: elfutils_default_options,
  include_type: 'system',
)

# Collect all the dependencies we require for HPCToolkit
boost_dep = dependency(
  'boost',
  'Boost',
  modules: ['graph'],
  version: '>=1.71.0',
  include_type: 'system',
)
libunwind_dep = dependency(
  'libunwind',
  version: ['>=1.6.2', '!=1.21'],
  default_options: [
    'minidebuginfo=enabled',
    'zlibdebuginfo=enabled',
    'warning_level=0',
    'werror=false',
  ],
  include_type: 'system',
)
xerces_dep = dependency(
  'xerces-c',
  version: '>=3.2.2',
  include_type: 'system',
  default_options: ['warning_level=0', 'werror=false'],
)
xxhash_dep = dependency('libxxhash', version: '>=0.8.1', include_type: 'system')
tbb_malloc_dep = dependency(
  'tbbmalloc',
  'TBB',
  components: ['tbb', 'tbbmalloc'],
  include_type: 'system',
)
yaml_cpp_dep = dependency(
  'yaml-cpp',
  version: '>=0.6.2',
  include_type: 'system',
  default_options: ['warning_level=0', 'werror=false'],
)
papi_dep = dependency(
  'papi',
  required: get_option('papi'),
  version: '>=5.6.0',
  include_type: 'system',
)
papi_has_components = false
if papi_dep.found()
  add_project_arguments('-DHPCRUN_SS_PAPI', language: ['c', 'cpp'])
  papi_has_components = cc.has_header_symbol(
    'papi.h',
    'PAPI_get_eventset_component',
    dependencies: papi_dep,
  )
  if papi_has_components
    add_project_arguments('-DHPCRUN_SS_PAPI_C_INTEL', language: ['c', 'cpp'])
  endif
endif


# These dependencies don't ship any dependency files, so we use CMake by default and fall back to
# compiler checks as a robustness case. The final fallback to dependency() in each stanza is to
# provide a better error message and enable wrap fallbacks.
libiberty_default_options = ['warning_level=0', 'werror=false']
libiberty_dep = dependency(
  'Libiberty',
  method: 'cmake',
  cmake_module_path: 'cmake',
  required: false,
  include_type: 'system',
  default_options: libiberty_default_options,
)
if not libiberty_dep.found()
  if cc.has_header('libiberty/demangle.h') or cc.has_header('demangle.h')
    _libiberty_lib = cc.find_library('iberty', required: false)
    if _libiberty_lib.found()
      libiberty_dep = declare_dependency(
        dependencies: _libiberty_lib,
        version: 'unknown',
      )
    endif
  endif
endif
if not libiberty_dep.found()
  # Force wrap fallback
  libiberty_dep = dependency(
    'libiberty',
    method: 'system',
    include_type: 'system',
    default_options: libiberty_default_options,
  )
endif

xed_default_options = ['warning_level=0', 'werror=false']
xed_dep = dependency('', required: false)
if host_machine.cpu_family() in ['x86', 'x86_64']
  xed_dep = dependency(
    'Xed',
    method: 'cmake',
    cmake_module_path: 'cmake',
    version: '>=2022.08.11',
    required: false,
    default_options: xed_default_options,
  )
  if not xed_dep.found()
    if cc.has_header('xed/xed-interface.h') or cc.has_header('xed-interface.h')
      _xed_lib = cc.find_library('xed', required: false)
      if _xed_lib.found()
        _xed_ver = cc.get_define(
          'XED_VERSION',
          prefix: '''
          #if __has_include(<xed/xed-interface.h>)
          #include <xed/xed-interface.h>
          #else
          #include <xed-interface.h>
          #endif
        ''',
          dependencies: _xed_lib,
        ).strip('"')
        if _xed_ver.startswith('v')
          _xed_ver = _xed_ver.substring(1)
        endif
        xed_dep = declare_dependency(dependencies: _xed_lib, version: _xed_ver)
      endif
    endif
  endif
  if not xed_dep.found()
    # Force wrap fallback
    xed_dep = dependency(
      'xed',
      method: 'system',
      version: '>=2022.08.11',
      default_options: xed_default_options,
    )
  endif
endif
if xed_dep.found()
  if xed_dep.type_name() == 'internal'
    assert(xed_dep.version() != 'undefined')
    if xed_dep.version().version_compare('>=2023.08.21')
      add_project_arguments('-DXED_DISPLACEMENT_INT64', language: ['c', 'cpp'])
    endif
  else
    foreach _prefix : ['xed/', '']
      if cc.has_header_symbol(
        f'@_prefix@xed-operand-values-interface.h',
        'xed_operand_values_get_branch_displacement_int64',
        dependencies: xed_dep,
      )
        add_project_arguments('-DXED_DISPLACEMENT_INT64', language: ['c', 'cpp'])
        break
      endif
    endforeach
  endif
endif

perfmon_dep = dependency(
  'Perfmon2',
  method: 'cmake',
  cmake_module_path: 'cmake',
  version: '>=4.0',
  required: false,
  include_type: 'system',
)
if not perfmon_dep.found()
  _perfmon_lib = cc.find_library(
    'pfm',
    has_headers: ['perfmon/pfmlib.h'],
    required: false,
  )
  if _perfmon_lib.found()
    perfmon_dep = declare_dependency(
      dependencies: _perfmon_lib,
      version: cc.get_define(
        'LIBPFM_VERSION',
        prefix: '#include <perfmon/pfmlib.h>',
        dependencies: _perfmon_lib,
      ),
    )
  endif
endif
if not perfmon_dep.found()
  perfmon_dep = dependency(
    'libpfm',
    method: 'system',
    version: '>=4.0',
    include_type: 'system',
  )
endif


# Dyninst ships config files, but up through 12.3 they aren't actually usable. We have a Find*
# script that fiddles with the exposed targets until it works, so if we can't get what we want
# directly use the find script to do so.
_dyninst_opts = [
  'openmp=enabled',
  'debuginfod=disabled',
  'warning_level=0',
  'werror=false',
]
dyninst_dep = dependency(
  'Dyninst',
  components: ['parseAPI', 'instructionAPI', 'symtabAPI'],
  modules: ['Dyninst::parseAPI', 'Dyninst::instructionAPI', 'Dyninst::symtabAPI'],
  version: '>=12.3',
  required: false,
  default_options: _dyninst_opts,
  include_type: 'system',
)
if not dyninst_dep.found()
  dyninst_dep = dependency(
    'Dyninst',
    method: 'cmake',
    cmake_module_path: 'cmake',
    components: ['parseAPI', 'instructionAPI', 'symtabAPI'],
    modules: ['Dyninst::parseAPI', 'Dyninst::instructionAPI', 'Dyninst::symtabAPI'],
    version: '>=12.3',
    default_options: _dyninst_opts,
    include_type: 'system',
  )
endif

# Set flags for features available in Dyninst
if dyninst_dep.type_name() == 'internal'
  assert(dyninst_dep.version() != 'undefined')
  if dyninst_dep.version().version_compare('>=13.0.0')
    add_project_arguments('-DUSE_GET_CONTAINING_MODULE', language: ['c', 'cpp'])
    if xed_dep.found()
      add_project_arguments('-DUSE_XED_FOR_GAPS', language: ['c', 'cpp'])
    endif
  endif
  if dyninst_dep.version().version_compare('>=11.0.0')
    add_project_arguments('-DDYNINST_SUPPORTS_INTEL_GPU', language: ['c', 'cpp'])
  endif
  if dyninst_dep.version().version_compare('>=10.0.0')
    add_project_arguments('-DDYNINST_USE_CUDA', language: ['c', 'cpp'])
  endif
else
  if xed_dep.found() and cpp.compiles(
    '''
#include <InstructionDecoder.h>
void test() {
  (void)Dyninst::InstructionAPI::InstructionDecoder::unknown_instruction::register_callback;
}
''',
    name: 'Dyninst::InstructionAPI::InstructionDecoder has unknown_instruction::register_callback',
    dependencies: dyninst_dep,
  )
    add_project_arguments('-DUSE_XED_FOR_GAPS', language: ['c', 'cpp'])
  endif
  if cpp.compiles(
    '''
#include <Symtab.h>
void test() {
  Dyninst::SymtabAPI::Symtab sym;
  sym.getContainingModule(42);
}
''',
    name: 'Dyninst::SymtabAPI::Symtab uses getContainingModule()',
    dependencies: dyninst_dep,
  )
    add_project_arguments('-DUSE_GET_CONTAINING_MODULE', language: ['c', 'cpp'])
  endif
  if cpp.has_header_symbol(
    'entryIDs.h',
    'intel_gpu_op_general',
    dependencies: dyninst_dep,
  )
    add_project_arguments('-DDYNINST_SUPPORTS_INTEL_GPU', language: ['c', 'cpp'])
  endif
  if cpp.compiles(
    '''
#include <Symtab.h>
void test() {
  (void)Dyninst::Arch_cuda;
}
''',
    name: 'Dyninst has Arch_cuda',
    dependencies: dyninst_dep,
  )
    add_project_arguments('-DDYNINST_USE_CUDA', language: ['c', 'cpp'])
  endif
endif
add_project_arguments(
  '-DENABLE_OPENMP',
  '-DENABLE_OPENMP_SYMTAB',
  language: ['c', 'cpp'],
)


# Find GoogleTest which is used as a test harness and framework. GoogleMock is a bundled library
# with extra features that extend the base GoogleTest experience.
gtest_dep = dependency(
  'gtest',
  version: '>=1.8.1',
  include_type: 'system',
  disabler: true,
  required: get_option('tests'),
)
gtest_main_dep = dependency(
  'gtest_main',
  version: '>=1.8.1',
  include_type: 'system',
  disabler: true,
  required: get_option('tests'),
)
gmock_dep = dependency(
  'gmock',
  version: '>=1.8.1',
  include_type: 'system',
  disabler: true,
  required: get_option('tests'),
)
gmock_main_dep = dependency(
  'gmock_main',
  version: '>=1.8.1',
  include_type: 'system',
  disabler: true,
  required: get_option('tests'),
)
foreach gtest_var : ['gtest_dep', 'gtest_main_dep', 'gmock_dep', 'gmock_main_dep']
  gdep = get_variable(gtest_var)
  if gdep.found() and gdep.version().version_compare('<1.11.0')
    # Before GoogleTest 1.11.0, a -lpthread argument is added to the pkg-config files
    # in the Cflags section. Some compilers (Clang) generate a warning when this
    # happens. Disable these warnings when the GoogleTest dependency is linked in.
    set_variable(
      gtest_var,
      declare_dependency(
        dependencies: gdep,
        version: gdep.version(),
        compile_args: cc.get_supported_arguments('-Wno-unused-command-line-argument'),
      ),
    )
  endif
endforeach


# Find rst2man, which is used to generate the man pages from source
rst2man = find_program('rst2man', 'rst2man.py', required: get_option('manpages'))


# Find MPI.
mpi_dep = dependency('MPI', language: 'cpp', required: get_option('hpcprof_mpi'))

# Meson 1.6.0 is affected by a bug where, if the above is an optional dependency
# (i.e. -Dhpcprof_mpi=auto) then the resulting dependency won't actually work.
# If it was found, do a compile check and then re-run dependency() if needed.
# See https://github.com/mesonbuild/meson/issues/13810.
if mpi_dep.found() and not cpp.links(
  files('meson/sanity.mpi.cpp'),
  dependencies: mpi_dep,
)
  mpi_dep = dependency('MPI', language: 'c')  # NB: Different language to dodge cache
endif

# MPICH tends to mix compile-time and linker-time arguments, despite having two
# different flags to fetch each. Some compilers (Clang) generate a warning when this
# happens. Disable these warnings when the MPI dependency is linked in.
if mpi_dep.found()
  mpi_dep = declare_dependency(
    dependencies: mpi_dep,
    version: mpi_dep.version(),
    compile_args: cc.get_supported_arguments('-Wno-unused-command-line-argument'),
    link_args: cc.get_supported_arguments('-Wno-unused-command-line-argument'),
  )
endif


# For testing hpcprof-mpi, we also need mpiexec to actually launch the tool.
mpiexec = find_program(
  'mpiexec',
  required: get_option('hpcprof_mpi_tests').require(
    mpi_dep.found(),
    error_message: 'hpcprof-mpi tests enabled but MPI was either disabled or not found',
  ),
)
if mpiexec.found()
  mpiexec_cmd = [mpiexec.full_path(), '-host', 'localhost:1000', '-n']
endif

# Find the Python installation, used for the Python unwinding support.
python = pymod.find_installation(
  'python3',
  required: get_option('python'),
  disabler: true,
)
if python.found() and python.language_version().version_compare('<3.8')
  # We only support Python 3.8+ for unwinding support
  python = disabler()
endif
if python.found()
  # We need access to the Python headers for unwinding support
  _py_dep = python.dependency(required: get_option('python'), include_type: 'system')
  if not _py_dep.found()
    python = disabler()
  endif
endif
if python.found()
  add_project_arguments('-DENABLE_LOGICAL_PYTHON', language: ['c', 'cpp'])
elif get_option('python').enabled()
  error('Python support requested but compatible Python install was not found')
endif


# Find support for CUDA performance monitoring
#
# NB: The CUDA dependency expects to be used with (implicit) `link_language: 'cuda'`, but we link
# it with `link_language: 'c'`. This causes link errors (`cannot find: -lcupti`) at build time.
# Fetching the dependency() before adding the 'cuda' language changes the search algorithm a bit and
# removes some fallbacks/checks, but it works when we link.
#
# There are a couple different Meson bugs and antifeatures at play here to cause this, but it works.

nvdisasm = find_program(
  'nvdisasm',
  '/usr/local/cuda/bin/nvdisasm',
  required: get_option('cuda'),
)
if nvdisasm.found()
  add_project_arguments(
    '-DCUDA_NVDISASM_PATH="@0@"'.format(
      nvdisasm.full_path().replace('\\', '\\\\').replace('"', '\\"'),
    ),
    language: ['c', 'cpp'],
  )
endif
cupti_dep = dependency('', required: false)
if get_option('cuda').allowed()
  cupti_dep = dependency(
    'CUDA',
    modules: ['cupti', 'cuda'],
    required: false,
    version: '>=11.2',
    include_type: 'system',
  )
  if cupti_dep.found()
    # NB: Meson's CUDA support doesn't check for headers. If the install is broken and is
    # missing headers, we won't catch the issue until the build fails. Check here, and if
    # we don't find them, defer to CMake's logic which does check for the headers.
    foreach header : ['cuda.h', 'cuda_runtime_api.h', 'cupti.h']
      if not cpp.has_header(header, dependencies: cupti_dep)
        cupti_dep = dependency('', required: false)
        break
      endif
    endforeach
  endif
  if not cupti_dep.found()
    cupti_dep = dependency(
      'CUDAToolkit',
      components: ['cupti'],
      modules: ['CUDA::cupti', 'CUDA::cuda_driver', 'CUDA::cudart_static'],
      required: get_option('cuda'),
      include_type: 'system',
    )
  endif
endif

# Note that we only need the CUDA language for the tests, so if cupti_dep wasn't found we don't
# enable this here either.
has_cuda = false
if cupti_dep.found()
  has_cuda = add_languages('cuda', native: false, required: get_option('cuda'))
  if not has_cuda
    cupti_dep = dependency('', required: false)
  endif
endif

if cupti_dep.found()
  add_project_arguments(
    '-DOPT_HAVE_CUDA',
    '-DENABLE_CUDA',
    '-DHPCRUN_SS_NVIDIA',
    '-DCUPTI_INSTALL_PREFIX="/nonexistent"',  # TODO
    language: ['c', 'cpp'],
  )
endif
if false  # get_option('papi_cupti')  # --enable-papi-c-cupti
  add_project_arguments('-DHPCRUN_SS_PAPI_C_CUPTI', language: ['c', 'cpp'])
endif

# Find support for ROCm performance monitoring
#hip_dep = dependency('hip', method: 'cmake', modules: ['hip::amdhip64'], required: get_option('rocm'), version: '>=5.1')

rocprofiler_dep = dependency('', required: false)
if get_option('rocm').allowed()
  rocprofiler_dep = dependency(
    'rocprofiler-sdk',
    method: 'cmake',
    cmake_module_path: 'cmake',
    required: false,
  )
  if not rocprofiler_dep.found()
    _rocprofiler_lib = cc.find_library(
      'rocprofiler-sdk',
      has_headers: ['rocprofiler-sdk/rocprofiler.h'],
      required: false,
    )
    if _rocprofiler_lib.found()
      # As a very special case, we also need the path to a metrics.xml file in the rocprofiler-sdk
      # installation. We don't have any paths at hand to find it otherwise, so we require the user
      # to specify the path via a project option. (We use an option to match the `c*_args` used for
      # the compiler checks, usually one would add these options to a machine file.)
      # if get_option('rocprofiler_metrics_xml') == ''
      #  error('-Drocprofiler_metrics_xml option must be set for rocprofiler-sdk')
      #endif
      #_metrics_xml = fs.expanduser(get_option('rocprofiler_metrics_xml'))
      #if not fs.exists(_metrics_xml)
      #  error(f'-Drocprofiler_metrics_xml set to an invalid path: no such file: @_metrics_xml@')
      #endif
      rocprofiler_dep = declare_dependency(
        dependencies: _rocprofiler_lib,
        # variables: {'metrics_xml': _metrics_xml},
        version: '@0@.@1@'.format(
          cc.get_define(
            'ROCPROFILER_VERSION_MAJOR',
            prefix: '#include <rocprofiler-sdk/version.h>',
            dependencies: _rocprofiler_lib,
          ),
          cc.get_define(
            'ROCPROFILER_VERSION_MINOR',
            prefix: '#include <rocprofiler-sdk/version.h>',
            dependencies: _rocprofiler_lib,
          ),
        ),
      )
    endif
  endif
  if not rocprofiler_dep.found()
    rocprofiler_dep = dependency(
      'rocprofiler-sdk',
      method: 'system',
      required: get_option('rocm'),
    )
  endif
endif

_rocm_subdeps = [rocprofiler_dep]
rocm_dep = declare_dependency(
  dependencies: _rocm_subdeps,
  # version: hip_dep.version(),
)
foreach dep : _rocm_subdeps
  if not dep.found()
    rocm_dep = dependency('', required: false)
    break
  endif
endforeach
if rocm_dep.found()
  add_project_arguments(
    '-DUSE_ROCM',
    '-DHPCRUN_SS_AMD',
    '-D__HIP_PLATFORM_AMD__',
    '-D__HIP_PLATFORM_HCC__',
    language: ['c', 'cpp'],
  )
endif
if false  # get_option('papi_rocm')  # --enable-papi-c-rocm
  add_project_arguments('-DHPCRUN_SS_PAPI_C_ROCM', language: ['c', 'cpp'])
endif

# Find HIP, the language used to test the ROCm support. Meson doesn't support it as a language, but
# we emulate Meson's basic behavior for languages.
# Similar to CUDA, we combine cpp_*args and hip_*args for the command lines here.
has_hip = false
if rocm_dep.found()
  _hip_cc = find_program('hip', 'hipcc', required: get_option('tests'))
  has_hip = _hip_cc.found()
  if has_hip
    hip = generator(
      _hip_cc,
      arguments: [
        get_option('cpp_args'),
        get_option('hip_args'),
        '-MD',
        '-MQ',
        '@OUTPUT@',
        '-MF',
        '@DEPFILE@',
        '-o',
        '@OUTPUT@',
        '-c',
        '@INPUT@',
      ],
      output: '@PLAINNAME@.o',
      depfile: '@PLAINNAME@.o.d',
    )
    hip_ld = [
      _hip_cc,
      get_option('cpp_link_args'),
      get_option('hip_link_args'),
      '-o',
      '@OUTPUT0@',
      '@INPUT@',
    ]
  endif
endif


# Find OpenCL support. We only need the headers, but there are many ways to acquire them.
opencl_dep = dependency('', required: false)
if get_option('opencl').allowed()
  _opencl_ver = '>=2.1'

  # - As of OpenCL >=2023.02.06 (found in Debian >=12, Ubuntu >=23.04, and Fedora) the
  #   headers can be accessed via pkg-config as OpenCL-Headers.pc.
  # - Before that, since OpenCL >=2020.12.18 (found in Debian >=10, Ubuntu >=20.04, and
  #   RHEL >=9) the headers can be accessed via CMake as OpenCLHeaders-config.cmake.
  opencl_dep = dependency(
    'OpenCL-Headers',
    'OpenCLHeaders',
    required: false,
    version: _opencl_ver,
    include_type: 'system',
  )

  # For older distros, CMake >=3.1 ships a FindOpenCL.cmake script, but it requires a
  # full OpenCL installation and not just the headers. If that's all we got, use it and
  # cut out the link flags so we only get the headers.
  if not opencl_dep.found()
    opencl_dep = dependency(
      'OpenCL',
      required: false,
      version: _opencl_ver,
      include_type: 'system',
    )
    opencl_dep = opencl_dep.partial_dependency(includes: true, compile_args: true)
  endif

  # For Spack builds and partial system installations, we also support the case where
  # the headers are already available on the standard include paths.
  if not opencl_dep.found() and cc.has_header('CL/cl.h', required: false)
    opencl_dep = declare_dependency()
  endif

  # Fall back to dependency() to get a good error message and enable wrap fallbacks.
  if not opencl_dep.found()
    opencl_dep = dependency(
      'OpenCL-Headers',
      required: get_option('opencl'),
      include_type: 'system',
    )
  endif
endif
if opencl_dep.found()
  add_project_arguments(
    '-DENABLE_OPENCL',
    '-DHPCRUN_SS_OPENCL',
    '-DCL_TARGET_OPENCL_VERSION=210',
    '-DCL_USE_DEPRECATED_OPENCL_1_2_APIS',
    language: ['c', 'cpp'],
  )
endif

# Find a full OpenCL installation. We need this to run some of the tests.
opencl_full_dep = disabler()
if opencl_dep.found()
  opencl_full_dep = dependency(
    'OpenCL',
    version: _opencl_ver,
    include_type: 'system',
    disabler: true,
    required: false,
  )

  # For badly written modulefiles, we also support the case where the library is already
  # available on environment-provided library paths.
  if not opencl_full_dep.found()
    opencl_full_dep = cc.find_library(
      'OpenCL',
      has_headers: ['CL/cl.h'],
      disabler: true,
      required: false,
    )
  endif

  # Fall back to dependency() to get a good error message and enable wrap fallbacks.
  if not opencl_full_dep.found()
    opencl_full_dep = dependency(
      'OpenCL',
      version: _opencl_ver,
      include_type: 'system',
      disabler: true,
      required: get_option('tests'),
    )
  endif
endif

# Find support for Intel performance monitoring via Level Zero.
level0_dep = dependency(
  'level-zero',
  required: get_option('level0'),
  version: '>=1.0',
  include_type: 'system',
)
if level0_dep.found()
  add_project_arguments('-DUSE_LEVEL0', '-DHPCRUN_SS_LEVEL0', language: ['c', 'cpp'])
  level0_api_prefix = '#include <ze_api.h>'
  if not cc.has_define(
    'ZE_API_VERSION_CURRENT_M',
    prefix: level0_api_prefix,
    dependencies: level0_dep,
  )
    major = cc.compute_int(
      'ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT)',
      prefix: level0_api_prefix,
      dependencies: level0_dep,
      low: 0,
      high: 10,
    )
    minor = cc.compute_int(
      'ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT)',
      prefix: level0_api_prefix,
      dependencies: level0_dep,
      low: 0,
      high: 100,
    )
    add_project_arguments(
      f'-DZE_API_VERSION_CURRENT_M=ZE_MAKE_VERSION(@major@, @minor@)',
      language: ['c', 'cpp'],
    )
  endif
endif

# Find support for instruction level collection, which requires IGC and IGA
_gtpin_f = get_option('gtpin').require(
  level0_dep.found(),
  error_message: 'gtpin is only available with level0',
)
iga_dep = dependency(
  'IGA',
  method: 'cmake',
  cmake_module_path: 'cmake',
  required: _gtpin_f,
  include_type: 'system',
)
if iga_dep.found()
  add_project_arguments('-DENABLE_IGA', language: ['c', 'cpp'])
endif

# GTPin is a snowflake of a case, it only comes in a binary tarball with non-standard
# names for all the paths like `Include/api` and `Lib/intel64`. This means we can't use
# CMake to find it, the user *has* to provide it through `cpp_args` and `cpp_link_args`.
# But since that's way too much trouble in most cases, we also support pulling it from a
# wrap, when the feature is explicitly requested.
gtpin_dep = dependency('', required: false)
if _gtpin_f.allowed()
  gtpin_dep = cpp.find_library(
    'gtpin',
    has_headers: ['gtpin_api.h', 'ged.h'],
    required: false,
  )
  if gtpin_dep.found()
    gtpin_dep = declare_dependency(dependencies: gtpin_dep, version: 'unknown')
  else
    gtpin_dep = dependency(
      'gtpin',
      method: 'system',
      required: _gtpin_f,
      include_type: 'system',
    )
  endif
endif
if gtpin_dep.found()
  add_project_arguments('-DENABLE_GTPIN', '-DUSE_GTPIN', language: ['c', 'cpp'])
endif

# Find SYCL, the language we use to test Level Zero support. Meson doesn't support it as a language,
# but we emulate Meson's basic behavior for languages.
# Similar to CUDA, we combine cpp_*args and hip_*args for the command lines here.
has_sycl = false
if level0_dep.found() and get_option('tests').allowed()

  # Path 1: If "sycl" has been registered as a binary in the machine file, use that.
  _sycl_cc = find_program('sycl', required: false)
  _sycl_args = []
  _sycl_link_args = []
  has_sycl = _sycl_cc.found()
  sycl_is_cpp = false

  # Path 2: If the C++ compiler supports compiling SYCL code, possibly with an extra flag, use that.
  # This path is a bit different because we use Meson's native C++ support instead of implementing
  # our own mess on top of things.
  if false and not has_sycl
    foreach sycl_dep : [
      declare_dependency(
        compile_args: [get_option('sycl_args')],
        link_args: [get_option('sycl_link_args')],
      ),
      declare_dependency(
        compile_args: ['-fsycl', get_option('sycl_args')],
        link_args: ['-fsycl', get_option('sycl_link_args')],
      ),
    ]
      if cpp.compiles(files('meson/sanity.sycl.cpp'), dependencies: sycl_dep)
        has_sycl = true
        sycl_is_cpp = true
        break
      endif
    endforeach
  endif

  # Path 3: Search for a known compiler that supports SYCL. These logics are tuned based on the
  # history and quirks of the compilers being attempted.
  if not has_sycl
    _icpx = find_program('icpx', required: false)
    if _icpx.found()
      run_command(
        build_python,
        files('meson/in-tmpdir.py'),
        _icpx,
        '-fsycl',
        '-c',
        '-o',
        'sanity.sycl.cpp.o',
        files('meson/sanity.sycl.cpp'),
        check: true,
      )
      _sycl_cc = _icpx
      _sycl_args = ['-fsycl']
      _sycl_link_args = ['-fsycl']
      has_sycl = true
    endif
  endif
  if not has_sycl
    _dpcpp = find_program('dpcpp', required: false)
    if _dpcpp.found()
      run_command(
        build_python,
        files('meson/in-tmpdir.py'),
        _dpcpp,
        '-c',
        '-o',
        'sanity.sycl.cpp.o',
        files('meson/sanity.sycl.cpp'),
        check: true,
      )
      _sycl_cc = _dpcpp
      _sycl_args = []
      _sycl_link_args = []
      has_sycl = true
    endif
  endif

  # Regardless of what route we take, if we need SYCL and don't have it, error out.
  if get_option('tests').enabled() and not has_sycl
    error('Unable to find a SYCL compiler!')
  endif

  if has_sycl and not sycl_is_cpp
    sycl = generator(
      _sycl_cc,
      arguments: [
        _sycl_args,
        get_option('cpp_args'),
        get_option('sycl_args'),
        '-MD',
        '-MQ',
        '@OUTPUT@',
        '-MF',
        '@DEPFILE@',
        '-o',
        '@OUTPUT@',
        '-c',
        '@INPUT@',
      ],
      output: '@PLAINNAME@.o',
      depfile: '@PLAINNAME@.o.d',
    )
    sycl_ld = [
      _sycl_cc,
      _sycl_link_args,
      get_option('cpp_link_args'),
      get_option('sycl_link_args'),
      '-o',
      '@OUTPUT0@',
      '@INPUT@',
    ]
  endif
endif


# Collect features from the compiler itself
math_dep = cc.find_library('m')
threads_dep = dependency('threads')
openmp_dep = dependency('openmp')
dl_dep = dependency('dl')
rt_dep = cc.find_library('rt')
_fs_code = '''
#include <filesystem>
int main() {
  std::filesystem::path p("/foo/bar");
  std::filesystem::remove(p);
  return 0;
}
'''
if not cpp.links(_fs_code)
  _found = false
  foreach trial : ['stdc++fs', 'c++fs']
    _dep = cpp.find_library(trial, required: false)
    if _dep.found()
      add_project_dependencies(_dep, language: ['c', 'cpp'])
      _found = true
      break
    endif
  endforeach
  if not _found
    error('Unable to find suitable flags for std::filesystem')
  endif
endif


# Add Valgrind headers for Valgrind annotations. If they aren't requested, inject a
# compiler flag to globally disable them.
add_project_dependencies(
  subproject('valgrind-headers').get_variable('valgrind_headers_dep'),
  language: ['c', 'cpp'],
)
if not get_option('valgrind_annotations')
  add_project_arguments('-DNVALGRIND', language: ['c', 'cpp'])
endif


# Expose the results of various compile-time checks
add_project_arguments(
  '-DSIZEOF_VOIDP=@0@'.format(
    cc.compute_int('sizeof(void*)', low: 1, high: 64, guess: 8),
  ),
  language: ['c', 'cpp'],
)

if host_machine.cpu_family() == 'x86'
  add_project_arguments('-DHOST_CPU_x86', language: ['c', 'cpp'])
elif host_machine.cpu_family() == 'x86_64'
  add_project_arguments('-DHOST_CPU_x86_64', language: ['c', 'cpp'])
elif host_machine.cpu_family() == 'ia64'
  add_project_arguments('-DHOST_CPU_IA64', language: ['c', 'cpp'])
elif host_machine.cpu_family() == 'ppc64'
  add_project_arguments('-DHOST_CPU_PPC', language: ['c', 'cpp'])
elif host_machine.cpu_family() == 'aarch64'
  add_project_arguments('-DHOST_CPU_ARM64', language: ['c', 'cpp'])
endif

if host_machine.endian() == 'big'
  add_project_arguments('-DHOST_BIG_ENDIAN', language: ['c', 'cpp'])
elif host_machine.endian() == 'little'
  add_project_arguments('-DHOST_LITTLE_ENDIAN', language: ['c', 'cpp'])
endif

if cc.has_header_symbol('byteswap.h', 'bswap_64')
  add_project_arguments('-DUSE_SYSTEM_BYTESWAP', language: ['c', 'cpp'])
endif
if cc.has_header_symbol(
  'linux/futex.h',
  'SYS_futex',
  prefix: ['#include <sys/syscall.h>', '#include <unistd.h>'],
)
  add_project_arguments('-DHAVE_FUTEX_H', language: ['c', 'cpp'])
endif

if cc.has_header_symbol('time.h', 'CLOCK_REALTIME') and cc.has_header_symbol(
  'signal.h',
  'SIGEV_THREAD_ID',
)
  add_project_arguments('-DENABLE_CLOCK_REALTIME', language: ['c', 'cpp'])
  if cc.has_header_symbol('time.h', 'CLOCK_THREAD_CPUTIME_ID')
    add_project_arguments('-DENABLE_CLOCK_CPUTIME', language: ['c', 'cpp'])
  endif
endif

has_perf_events = cc.has_type(
  'struct perf_event_attr',
  prefix: [
    '#include <linux/perf_event.h>',
    '#include <linux/hw_breakpoint.h>',
    '#include <sys/syscall.h>',
    '#include <unistd.h>',
  ],
)
if has_perf_events
  add_project_arguments('-DHPCRUN_SS_LINUX_PERF', language: ['c', 'cpp'])
endif


# Inject some strings into the build
add_project_arguments(
  '-DHPCTOOLKIT_INSTALL_PREFIX="@0@"'.format(
    get_option('prefix').replace('\\', '\\\\').replace('"', '\\"'),
  ),
  language: ['c', 'cpp'],
)


subdir('tools')
subdir('doc')
subdir('src')
subdir('tests')
