# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# pylint: disable=fixme,invalid-name

"Sphinx configuration for this documentation tree"

import os
from pathlib import PurePath


def path_to_url(path: PurePath) -> str:
    "Convert a Path into a suitable URL for HTML purposes"
    return ("/" if path.is_absolute() else "") + "/".join(path.parts[1:])


def abs_url(base: str, path: str) -> str:
    "Generate the absolute URL given the base"
    path = path.lstrip("/")
    if base and not base.endswith("/"):
        return f"{base}/{path}"
    return (base or "/") + path


# Project settings
project = "HPCToolkit"
assert project.lower() == os.environ["CONF_PROJECT_NAME"]
project_copyright = "HPCToolkit Project a Series of LF Projects, LLC"
author = f"The {project} Developers"
release = os.environ["CONF_VERSION"]
version = release


suppress_warnings = [
    # FIXME: The EPUB builder tries to include ALL files in the build directory, not just
    # the ones produced by Sphinx. This tends to cause warnings when the files aren't of a
    # supported MIME type. For now, ignore this kind of warning entirely.
    "epub.unknown_project_files",
]


# Configuration for the sources, which are primarily written in MyST
language = "en"
master_doc = "index"
extensions = ["myst_parser"]
myst_enable_extensions = [
    "deflist",
    "replacements",
    "smartquotes",
]


# Configuration for the HTML output
html_theme = "sphinx_book_theme"
html_title = "HPCToolkit"
html_logo = "branding/logo.svg"
html_favicon = "branding/favicon.svg"
html_baseurl = os.environ.get("CONF_HTML_BASEURL") or path_to_url(
    PurePath(os.environ["CONF_INSTALL_PREFIX"])
)
html_static_path = ["branding/hpcviewer.svg"]
html_sidebars = {
    "**": [
        "navbar-logo",
        "icon-links",
        "search-button-field",
        "sbt-sidebar-nav",
    ]
}
html_theme_options: dict = {
    "icon_links": [
        {
            "name": "HPCViewer",
            "url": abs_url(html_baseurl, "users/hpcviewer/hpcviewer.html"),
            "type": "local",
            "icon": "_static/hpcviewer.svg",
        },
        {
            "name": "Download as EPUB",
            "url": abs_url(html_baseurl, project.lower() + ".epub"),
            "type": "fontawesome",
            "icon": "fa-solid fa-book",
        },
    ],
    "repository_url": "https://gitlab.com/hpctoolkit/hpctoolkit",
    "repository_branch": "develop",
    "path_to_docs": "doc/src/",
    "use_edit_page_button": True,
    "use_source_button": True,
    "use_issues_button": True,
}

if os.environ["CONF_HAS_PDF"] == "true":
    html_theme_options["icon_links"].append(  # type: ignore
        {
            "name": "Download as PDF",
            "url": abs_url(html_baseurl, project.lower() + ".pdf"),
            "type": "fontawesome",
            "icon": "fa-regular fa-file-pdf",
        }
    )

if os.environ["CONF_VERSIONS_URL"]:
    html_theme_options["switcher"] = {
        "version_match": release,
        "json_url": os.environ["CONF_VERSIONS_URL"],
    }
    html_theme_options["show_version_warning_banner"] = True
    html_sidebars["**"].append("version-switcher")


# Configuration for the EPUB output
epub_basename = project.lower()
