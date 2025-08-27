# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# pylint: disable=invalid-name,missing-module-docstring

import os.path
import sys

# Basic project configuration
project_copyright = "HPCToolkit Project a Series of LF Projects, LLC"

# Sphinx configuration options
language = "en"
nitpicky = True
master_doc = "index"

html_title = 'HPCToolkit'
html_logo = 'hpctoolkit-wordmark.png'

# Enable extensions. Custom extensions are under _ext/
sys.path.insert(0, os.path.abspath("./_ext"))
extensions = ["myst_parser", "sphinx_depfile"]
needs_extensions = {"myst_parser": "0.16"}

# Configuration for the (primary) HTML output.
html_theme = "sphinx_book_theme"

# Configuration for MyST, the Markdown parser for Sphinx.
myst_enable_extensions = [
    "deflist",
    "replacements",
    "smartquotes",
]
