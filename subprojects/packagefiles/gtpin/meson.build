# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

project('gtpin', 'cpp', version: '4.5.0', meson_version: '>=1.0.0')

if not (host_machine.system() == 'linux' and host_machine.cpu_family() == 'x86_64')
  error('GTPin is only available on Linux x86-64, and will not work for your system')
endif

cpp = meson.get_compiler('cpp')
incdirs = include_directories('Profilers/Include/api', 'Profilers/Include/ged/intel64')
gtpin_dep = declare_dependency(
  dependencies: cpp.find_library(
    'gtpin',
    dirs: meson.current_source_dir() / 'Profilers/Lib/intel64',
  ),
  include_directories: incdirs,
  version: meson.project_version(),
)

# Check that the include directories are correct and the headers actually work.
foreach h : ['gtpin_api.h', 'ged.h']
  assert(cpp.check_header(h, dependencies: gtpin_dep, include_directories: incdirs))
endforeach

meson.override_dependency('gtpin', gtpin_dep)
