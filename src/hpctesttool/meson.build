# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: BSD-3-Clause

# This directory provides `hpctesttool`, a super-command for a series of helpful tools for writing
# precise and robust test-scripts. And other debugging tool.

hpctesttool_venv = custom_target(
  'hpctesttool',
  output: ['hpctesttool', 'python'],
  build_by_default: false,
  command: [
    python,
    files('install.py'),
    '@PRIVATE_DIR@',
    subproject('hpctesttool-wheels').get_variable('wheels'),
    '@CURRENT_SOURCE_DIR@',
    '@OUTPUT@',
  ],
  depend_files: [files('pyproject.toml')],
)
meson.add_devenv({'PATH': meson.current_build_dir()}, method: 'prepend')

hpctesttool = hpctesttool_venv[0]

# Also run pytest to test the utilities themselves
_env = environment()
_env.set('TEST_DATA_DIR', meson.current_source_dir() / '../../tests/data')
test(
  'pytest',
  find_program('sh'),
  timeout: 0,
  protocol: 'gtest',
  verbose: true,
  env: _env,
  args: [
    # FIXME: Workaround for Meson 1.1. This can be removed when we update to Meson 1.4.
    # https://mesonbuild.com/Release-notes-for-1-4-0.html#use-custom_target-as-test-executable
    '-c',
    'filename="$1"; shift; exec "$(realpath "$filename")" "$@"',
    'sh',
    hpctesttool_venv[1],
    files('pytest-as-gtest.py'),
    '--override-ini=cache_dir=@0@'.format(meson.current_build_dir() / '.pytest_cache'),
    meson.current_source_dir() / 'hpctesttool',
  ],
  workdir: meson.current_build_dir(),
)
