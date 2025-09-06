# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

fs = import('fs')

_eu_headers = [fs.copyfile('../libdw.h'), fs.copyfile('../known-dwarf.h')]

install_headers('../libdw.h', '../known-dwarf.h', subdir: 'elfutils')
