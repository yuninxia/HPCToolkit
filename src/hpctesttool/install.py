#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import shutil
import stat
import subprocess
import venv
from pathlib import Path


def create_venv(out: Path) -> None:
    shutil.rmtree(out, ignore_errors=True)
    venv.EnvBuilder(system_site_packages=False, clear=True, with_pip=True).create(out)


def pip(*args, venv: Path, wheels: Path) -> None:
    subprocess.run(
        [
            venv / "bin" / "python",
            "-m",
            "pip",
            "--no-input",  # Don't prompt for input
            "--no-cache-dir",  # Don't use the cache
            "--disable-pip-version-check",  # Don't check the PyPI for newer versions of Pip itself
            "--require-virtualenv",  # Ensure we're running in the virtual environment
            "install",
            "--no-warn-script-location",  # Don't warn about bin/ not being on the PATH
            "--no-index",  # Don't query the PyPI for anything
            "--use-pep517",  # Force PEP 517 when building from source
            f"--find-links={wheels!s}",  # Pull dependencies from this vendored directory
            *args,
        ],
        check=True,
    )


def create_redirect(venv: Path, filename: str, out: Path) -> None:
    out.unlink(missing_ok=True)
    out.write_text(
        f"""#!/bin/sh
exec {(venv / "bin" / filename).resolve(strict=True)!s} "$@"
""",
        encoding="utf-8",
    )
    out.chmod(
        (out.stat().st_mode & stat.S_IRWXU) | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    )


def arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("venv", type=Path)
    parser.add_argument("wheels", type=Path)
    parser.add_argument("src", type=Path)
    parser.add_argument("hpctesttool", type=Path)
    parser.add_argument("python", type=Path)
    return parser


def main() -> None:
    args = arg_parser().parse_args()
    create_venv(args.venv)
    pip("--upgrade", "pip", venv=args.venv, wheels=args.wheels)
    pip("poetry.core", venv=args.venv, wheels=args.wheels)
    pip(
        "--force-reinstall",  # Always reinstall the tool if we make it here
        "--editable",  # Ensure updates to the source propagate to the installation
        args.src,
        venv=args.venv,
        wheels=args.wheels,
    )
    create_redirect(args.venv, "hpctesttool", args.hpctesttool)
    create_redirect(args.venv, "python", args.python)


if __name__ == "__main__":
    main()
