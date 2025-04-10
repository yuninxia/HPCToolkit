# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

project(
  'md5',
  'c',
  version: 'g72cf2cd',
  meson_version: '>=1.0.0',
  default_options: ['c_std=c99'],
)

md5_lib = static_library('md5', 'md5.c', pic: true)
md5_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: md5_lib,
)

tests = executable('tests', 'tests.c', link_with: md5_lib)
test('tests', tests, args: ['--test'])
