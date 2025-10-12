# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# This directory provides `hpctesttool`, a super-command for a series of helpful tools for writing
# precise and robust test-scripts. And other debugging tool.

# Install the tool inside a virtual environment
hpctesttool_venv = meson.project_build_root() / meson.current_build_dir() / 'venv'
run_command(
  build_python,
  '-m',
  'venv',
  ensurepip_found ? [] : '--without-pip',
  '--upgrade',
  hpctesttool_venv,
  capture: false,
  check: true,
)
hpctesttool_python = find_program(hpctesttool_venv / 'bin' / 'python', native: true)
# NB: All the wheels needed to use the test tool are vendored as a subproject. This
# means we don't want to access the PyPI (--no-index), and instead only use our
# predownloaded wheels to satisfy dependencies (--find-links).
run_command(
  hpctesttool_python,
  ensurepip_found ? ['-m', 'pip'] : pip,
  '--no-input',
  '--disable-pip-version-check',
  '--require-virtualenv',
  'install',
  '--no-warn-script-location',
  '--no-index',
  '--find-links=@0@'.format(subproject('hpctesttool-wheels').get_variable('wheels')),
  meson.project_source_root() / meson.current_source_dir(),
  env: {'PIP_FIND_LINKS': '', 'PIP_EXTRA_INDEX_URL': ''},
  check: true,
)

# Also run pytest to test the utilities themselves
_env = environment()
_env.set('TEST_DATA_DIR', meson.current_source_dir() / '../../tests/data')
test(
  'pytest',
  hpctesttool_python,
  protocol: 'gtest',
  verbose: true,
  env: _env,
  args: [
    files('pytest-as-gtest.py'),
    '--override-ini=cache_dir=@0@'.format(meson.current_build_dir() / '.pytest_cache'),
    meson.current_source_dir() / 'hpctesttool',
  ],
  workdir: meson.current_build_dir(),
  suite: 'hpctesttool',
)

# Generate a simple script that will run the tool, and expose it inside the devenv.
hpctesttool = configure_file(
  input: 'hpctesttool.in.sh',
  output: 'hpctesttool',
  configuration: {'HPCTESTTOOL': hpctesttool_venv / 'bin' / 'hpctesttool'},
)
meson.add_devenv({'PATH': meson.current_build_dir()}, method: 'prepend')
