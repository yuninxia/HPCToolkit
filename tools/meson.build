# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Tool to help construct virtual environments from requirements.{in,txt}.
native_pyvenv = [build_python, files('make-venv.py')]
if get_option('wrap_mode') == 'default'
  native_pyvenv += ['--allow-fallback-install']
endif
