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

# PDF generation requires Latexmk
latexmk = find_program(
  'latexmk',
  native: true,
  required: get_option('manual_pdf'),
  disabler: true,
)

sphinx_args = ['-q', '-W', '--keep-going', '--jobs=auto', '@CURRENT_SOURCE_DIR@']
sphinx_env = {
  'CONF_PROJECT_NAME': meson.project_name(),
  'CONF_VERSION': meson.project_version(),
  'CONF_INSTALL_PREFIX': get_option('prefix') / docdir,
  'CONF_HTML_BASEURL': get_option('docs_host_url'),
  'CONF_HAS_PDF': latexmk.found() ? 'true' : '',
}

# The main documentation output is static, multi-file HTML
docs_html = custom_target(
  output: 'index.html',
  command: [sphinx, '-bhtml', sphinx_args, '@OUTDIR@'],
  env: sphinx_env,
  console: true,
  build_always_stale: true,
)
meson.add_install_script(
  native_python,
  files('install_html.py'),
  docs_html,
  docdir,
  dry_run: true,
  install_tag: 'docs',
)

# For offline reading, the recommended format is EPUB. This is essentially the HTML
# site repackaged as a single file.
docs_epub = custom_target(
  output: '@0@.epub'.format(meson.project_name().to_lower()),
  command: [sphinx, '-bepub', sphinx_args, '@OUTDIR@'],
  env: sphinx_env,
  console: true,
  build_always_stale: true,
  install: true,
  install_tag: 'docs',
  install_dir: docdir,
)

# For offline reading, a very common format is PDF. Sphinx doesn't support PDF output
# directly, instead it can generate Latex which can be rendered as a PDF.
docs_latex = custom_target(
  output: '@0@.tex'.format(meson.project_name().to_lower()),
  command: [sphinx, '-blatex', sphinx_args, '@OUTDIR@'],
  env: sphinx_env,
  console: true,
  build_always_stale: true,
)
docs_pdf = custom_target(
  output: '@0@.pdf'.format(meson.project_name().to_lower()),
  input: docs_latex,
  command: [latexmk, '-cd', '-pdf', '-dvi-', '-ps-', '@INPUT@'],
  console: true,
  build_always_stale: true,
  install: true,
  install_tag: 'docs',
  install_dir: docdir,
)

# Helper target to build all the documentation at once
docs_all = [docs_html, docs_epub]
if not is_disabler(docs_pdf)
  docs_all += docs_pdf
endif
alias_target('docs', docs_all)

# Helper target to serve the built docs over HTTP for development
run_target(
  'docs-serve',
  command: [
    native_python,
    files('serve.py'),
    meson.current_build_dir(),
    get_option('prefix') / docdir,
  ],
  depends: docs_all,
)
