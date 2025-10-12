# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project('pip', version: '25.0.1', meson_version: '>=1.0.0')

# Confirm the sources match the version reported in the wrap
fs = import('fs')
distinfo = 'pip-@0@.dist-info'.format(meson.project_version())
assert(fs.is_dir(distinfo), 'Wrap version needs updated')

# Expose the path to Pip as a variable for the parent project to use
# Usage: [find_program('python3'), pip, 'install', ...]
pip = meson.project_source_root() / 'pip'
