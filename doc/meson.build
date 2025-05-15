# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: BSD-3-Clause

docdir = get_option('datadir') / 'doc' / meson.project_name()

install_data('FORMATS.md', 'METRICS.yaml', install_dir: docdir, install_tag: 'docs')

dtd_hpc_structure = files('dtd/hpc-structure.dtd')

install_data(
  'dtd/hpc-structure.dtd',
  install_dir: get_option('datadir') / meson.project_name() / 'dtd',
  install_tag: 'dtd',
)

subdir('src')
subdir('man')
