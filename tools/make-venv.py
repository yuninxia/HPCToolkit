#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from pathlib import Path
import venv
import subprocess
import urllib.request
import tempfile
import sys
import json

try:
    import ensurepip

    has_ensurepip = True
except ModuleNotFoundError:
    has_ensurepip = False

try:
    import pip

    has_pip = True
except ModuleNotFoundError:
    has_pip = False


def parse_arguments():
    parser = argparse.ArgumentParser(description="Create a Python virtual environment")
    parser.add_argument(
        "--allow-fallback-install",
        action="store_true",
        help="Install from the PyPI as a fallback",
    )
    parser.add_argument(
        "--skip-if-satisfied",
        action="store_true",
        help="Skip venv creation if requirements are already satisfied",
    )
    parser.add_argument(
        "venv", type=Path, help="Path to the virtual environment to create"
    )
    parser.add_argument(
        "requirements",
        type=Path,
        help="requirements.in to ensure are available in the final environment",
    )
    parser.add_argument(
        "pinned_requirements",
        type=Path,
        help="Pinned requirements.txt to install from the PyPI as a fallback",
    )
    return parser.parse_args()


def check_missing_requirements(requirements: Path, python: Path | None) -> list[str]:
    result = subprocess.run(
        [
            python or sys.executable,
            "-m",
            "pip",
            "install",
            "--dry-run",
            "--quiet",
            "--report",
            "-",
            "-r",
            requirements,
        ],
        check=True,
        capture_output=True,
        encoding="utf-8",
    )
    report = json.loads(result.stdout)
    if report.get("version", 0) != "1":
        raise RuntimeError(
            f"Invalid report format, output is v{report.get('version', 0)} not v1"
        )
    return [
        f"{m['metadata']['name']}=={m['metadata']['version']}"
        for m in report["install"]
    ]


def setup_venv(
    output: Path, *, allow_fallback: bool, system_site_packages: bool = False
):
    """Set up a virtual environment with Pip preinstalled"""
    venv.create(output, with_pip=False, system_site_packages=system_site_packages)
    venv_python = output / "bin" / "python"

    if system_site_packages and has_pip:
        # Venv can access the system's copy of Pip
        pass
    elif subprocess.run([venv_python, "-c", "import pip"], check=False).returncode == 0:
        pass
    elif has_ensurepip:
        subprocess.run([venv_python, "-m", "ensurepip"], check=True)
    elif allow_fallback:
        print("Downloading bootstrap Pip from the PyPA...")
        with tempfile.NamedTemporaryFile() as temp:
            urllib.request.urlretrieve(
                "https://bootstrap.pypa.io/get-pip.py", temp.name
            )
            subprocess.run([output / "bin" / "python", temp.name], check=True)
    else:
        raise RuntimeError(
            "Failed to bootstrap Pip, install ensurepip or allow wrap fallbacks"
        )


def main():
    args = parse_arguments()

    # Determine if the requirements are already satisfied on the system
    if args.skip_if_satisfied and has_pip:
        to_install = check_missing_requirements(args.requirements, None)
        if not to_install:
            print("Requirements are already satisfied, skipping venv creation")
            sys.exit(7)
        print(
            f"Requirements not yet satisifed, would install: {', '.join(to_install)}",
            file=sys.stderr,
        )
    else:
        setup_venv(
            args.venv,
            allow_fallback=args.allow_fallback_install,
            system_site_packages=True,
        )
        to_install = check_missing_requirements(
            args.requirements, args.venv / "bin" / "python"
        )
        if not to_install:
            print("Requirements are satisfied")
            sys.exit(7 if args.skip_if_satisfied else 0)
        print(
            f"Requirements not yet satisifed, would install: {', '.join(to_install)}",
            file=sys.stderr,
        )

    # If fallbacks are disallowed, print an error and exit early
    if not args.allow_fallback_install:
        print("Fallback dependencies are disabled, unable to continue", file=sys.stderr)
        sys.exit(1)

    # Set up and install the full venv from scratch
    setup_venv(args.venv, allow_fallback=args.allow_fallback_install)
    subprocess.run(
        [
            args.venv / "bin" / "python",
            "-m",
            "pip",
            "install",
            "-r",
            args.pinned_requirements,
            "-r",
            args.requirements,
        ],
        check=True,
    )


if __name__ == "__main__":
    main()
