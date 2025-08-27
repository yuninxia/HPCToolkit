# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

manual_f = get_option('manual').enable_if(
  get_option('manual_pdf').enabled(),
  error_message: 'PDF manual cannot be enabled without HTML manual',
)
if not manual_f.allowed()
  subdir_done()
endif

# Find or install Sphinx and the necessary dependencies
sphinx_venv = meson.current_build_dir() / '.venv'
if native_python.found()
  sphinx_venv_result = run_command(
    [
      native_pyvenv,
      '--skip-if-satisfied',
      sphinx_venv,
      files('requirements.in', 'requirements.txt'),
    ],
    check: false,
  )
  if sphinx_venv_result.returncode() == 0
    sphinx = sphinx_venv / 'bin' / 'sphinx-build'
    message('Program sphinx-build installed from PyPI')
  elif sphinx_venv_result.returncode() == 7
    sphinx = find_program(
      'sphinx-build',
      native: true,
      required: manual_f,
      disabler: true,
    )
  else
    sphinx = disabler()
  endif
else
  sphinx = disabler()
endif

if is_disabler(sphinx) and manual_f.enabled()
  error('Sphinx is required for manual generation, but was not found nor installed')
endif

_project_name = 'HPCToolkit'
assert(_project_name.to_lower() == meson.project_name())

_sphinx_args = [
  '-q',
  '-E',
  # '-W',
  '--keep-going',
  '-d',
  '@PRIVATE_DIR@/.doctrees',
  '-Dproject=@0@'.format(_project_name),
  '-Dauthor=The @0@ Developers'.format(_project_name),
  '-Dversion=@0@'.format(meson.project_version()),
  '-Drelease=@0@'.format(meson.project_version()),
  '-Ddepfile=@DEPFILE@',
  '@CURRENT_SOURCE_DIR@',
]
_sphinx_depends = files('conf.py')

# The main documentation is generated as a static HTML webpage.
custom_target(
  output: 'html',
  depfile: 'html.d',
  command: [sphinx, '-bhtml', _sphinx_args, '@OUTPUT@'],
  depend_files: _sphinx_depends,
  install: true,
  install_dir: docdir,
  install_tag: 'docs',
)

# An alternative format is EPUB, which is (essentially) a zip file containing HTML.
# This format may be slightly nicer to copy across machines when necessary.
custom_target(
  output: '@0@.epub'.format(meson.project_name()),
  depfile: '@0@.epub.d'.format(meson.project_name()),
  command: [
    native_python,
    files('sphinx-singlefile.py'),
    '@PRIVATE_DIR@/build',
    f'@_project_name@.epub',
    '@OUTPUT@',
    sphinx,
    '-bepub',
    _sphinx_args,
  ],
  depend_files: _sphinx_depends,
  install: true,
  install_dir: docdir,
  install_tag: 'docs',
)

# A more common format for offline documentation is PDF, but this requires a fairly complete
# Latex installation, so it has a separate option and is disabled by default.
latexmk = find_program(
  'latexmk',
  native: true,
  required: get_option('manual_pdf'),
  disabler: true,
)
custom_target(
  output: '@0@.pdf'.format(meson.project_name()),
  depfile: '@0@.pdf.d'.format(meson.project_name()),
  command: [
    native_python,
    files('sphinx-pdf.py'),
    '@PRIVATE_DIR@/build',
    '@0@.pdf'.format(_project_name.to_lower()),
    '@OUTPUT@',
    latexmk,
    '@0@.tex'.format(_project_name.to_lower()),
    sphinx,
    '-blatex',
    _sphinx_args,
  ],
  depend_files: _sphinx_depends,
  install: true,
  install_dir: docdir,
  install_tag: 'docs',
)
