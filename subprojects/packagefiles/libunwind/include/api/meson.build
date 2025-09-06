# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

fs = import('fs')

foreach f : _headers
  configure_file(output: fs.name(f), input: f, copy: true)
endforeach

iface_inc = include_directories('.')
