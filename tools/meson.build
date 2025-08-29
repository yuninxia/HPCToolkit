# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Native Python, used for many of the tool operations in this directory.
native_python = find_program(
  'python3',
  'python',
  native: true,
  version: '>=3.8',
  required: false,
)
if not native_python.found()
  native_python_names = []
  foreach ver : range(8, 15)
    native_python_names += [f'python3.@ver@']
  endforeach
  native_python = find_program(
    f'python3.@ver@',
    native: true,
    version: '>=3.8',
    required: false,
  )
endif

# Tool to help construct virtual environments from requirements.{in,txt}.
if native_python.found()
  native_pyvenv = [native_python, files('make-venv.py')]
  if get_option('wrap_mode') == 'default'
    native_pyvenv += ['--allow-fallback-install']
  endif
endif
