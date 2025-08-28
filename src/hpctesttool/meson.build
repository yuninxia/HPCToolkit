# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# This directory provides `hpctesttool`, a super-command for a series of helpful tools for writing
# precise and robust test-scripts. And other debugging tool.

# Install the test tool inside a Python virtual environment, for easy dependency management
hpctesttool_python = custom_target(
  output: 'hpctesttool-python',
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

# Generate a simple script that will defer to the hpctesttool module. We use a
# custom_target here to propagate the dependency on hpctesttool_python.
# NB: hpctesttool needs to be a custom_target (not custom_idx) to dodge
# https://github.com/mesonbuild/meson/issues/14743
hpctesttool_generator = '''
import pathlib, sys
out = pathlib.Path(sys.argv[1])
out.unlink(missing_ok=True)
out.write_text(f"""#!/bin/sh
exec {sys.executable} -m hpctesttool "$@"
""")
out.chmod(out.stat().st_mode | 0o111)
'''
hpctesttool = custom_target(
  output: 'hpctesttool',
  command: [hpctesttool_python, '-c', hpctesttool_generator, '@OUTPUT@'],
  build_by_default: false,
)
meson.add_devenv({'PATH': meson.current_build_dir()}, method: 'prepend')

# Also run pytest to test the utilities themselves
_env = environment()
_env.set('TEST_DATA_DIR', meson.current_source_dir() / '../../tests/data')
test(
  'pytest',
  find_program('sh'),
  protocol: 'gtest',
  verbose: true,
  env: _env,
  args: [
    # FIXME: Workaround for Meson 1.1. This can be removed when we update to Meson 1.4.
    # https://mesonbuild.com/Release-notes-for-1-4-0.html#use-custom_target-as-test-executable
    '-c',
    'filename="$1"; shift; exec "$(realpath "$filename")" "$@"',
    'sh',
    hpctesttool_python,
    files('pytest-as-gtest.py'),
    '--override-ini=cache_dir=@0@'.format(meson.current_build_dir() / '.pytest_cache'),
    meson.current_source_dir() / 'hpctesttool',
  ],
  workdir: meson.current_build_dir(),
  suite: 'hpctesttool',
)
