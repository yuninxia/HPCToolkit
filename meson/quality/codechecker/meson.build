# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# Find CodeChecker for running clang-tidy etc.
codechecker_force_fb = 'codechecker' in get_option('force_fallback_for') or 'CodeChecker' in get_option(
  'force_fallback_for',
) or get_option(
  'wrap_mode',
) == 'forcefallback'
codechecker = disabler()
if not codechecker_force_fb
  codechecker = find_program(
    'CodeChecker',
    version: '>=6.25.0',
    version_argument: 'version',
    native: true,
    required: false,
    disabler: true,
  )
endif

# If needed, install a fallback CodeChecker. This is usually more convenient than
# installing CodeChecker by hand.
if codechecker_force_fb or not codechecker.found() and get_option('dev_quality').enabled() and get_option(
  'wrap_mode',
) != 'nofallback'
  message('Installing CodeChecker in a virtual environment with Pip...')
  codechecker_venv = meson.project_build_root() / meson.current_build_dir() / '.venv'
  run_command(
    build_python,
    '-m',
    'venv',
    ensurepip_found ? [] : '--without-pip',
    '--upgrade',
    codechecker_venv,
    check: true,
  )
  run_command(
    codechecker_venv / 'bin' / 'python',
    ensurepip_found ? ['-m', 'pip'] : pip,
    '--no-input',
    '--disable-pip-version-check',
    '--require-virtualenv',
    'install',
    '--no-warn-script-location',
    '--require-hashes',
    get_option('wrap_mode') == 'nodownload' ? '--no-index' : [],
    '--requirement',
    files('requirements.txt'),
    check: true,
  )
  codechecker = find_program(codechecker_venv / 'bin' / 'CodeChecker', native: true)
endif

# Generate a "skipfile" to guide CodeChecker's analysis towards just the code in this
# project and not in parent or subprojects.
codechecker_skipfile = configure_file(
  input: 'skipfile.in',
  output: 'skipfile',
  configuration: {
    'MESON_GLOBAL_SOURCE_ROOT': meson.global_source_root(),
    'MESON_PROJECT_SOURCE_ROOT': meson.project_source_root(),
    'MESON_GLOBAL_BUILD_ROOT': meson.global_build_root(),
    'MESON_PROJECT_BUILD_ROOT': meson.project_build_root(),
  },
)
codechecker_options = [
  '--skip',
  codechecker_skipfile,
  '--config',
  files('../../../.codechecker.json'),
]

codechecker_collect_trg = run_target(
  'quality-codechecker-collect',
  command: [
    codechecker,
    'analyze',
    codechecker_options,
    '--clean',
    '--output=' + meson.current_build_dir() / 'logs',
    meson.global_build_root() / 'compile_commands.json',
  ],
)
alias_target('codechecker-collect', codechecker_collect_trg)
quality_collect_targets += codechecker_collect_trg

codechecker_codeclimate_trg = custom_target(
  'quality-codechecker-codeclimate',
  command: [
    build_python,
    files('fix_exit_status.py'),
    codechecker,
    'parse',
    codechecker_options,
    '--trim-path-prefix=' + meson.global_source_root(),
    '--export=codeclimate',
    '--output=@OUTPUT@',
    meson.current_build_dir() / 'logs',
  ],
  output: 'codechecker.codeclimate.json',
  build_by_default: false,
  build_always_stale: true,
)
alias_target('codechecker-codeclimate', codechecker_codeclimate_trg)
quality_codeclimate_targets += codechecker_codeclimate_trg
