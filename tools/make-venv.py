#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# pylint: disable=invalid-name,missing-module-docstring

import argparse
import importlib.util
import json
import subprocess
import sys
import tempfile
import typing
import urllib.request
import venv
from pathlib import Path

HAS_ENSUREPIP = importlib.util.find_spec("ensurepip") is not None
HAS_PIP = importlib.util.find_spec("pip") is not None


def parse_arguments() -> argparse.Namespace:
    """Parse command line arguments for virtual environment creation.

    This function sets up and processes command-line arguments for creating a Python virtual
    environment with specific package requirements.

    Returns:
        argparse.Namespace: Parsed command-line arguments containing:
            - allow_fallback_install (bool): Whether to allow PyPI fallback installation
            - skip_if_satisfied (bool): Whether to skip venv creation if requirements are met
            - venv (Path): Path where virtual environment will be created
            - requirements (Path): Path to requirements.in file
            - pinned_requirements (Path): Path to pinned requirements.txt file
    """

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


def check_missing_requirements(
    requirements: Path, python: Path | None
) -> typing.List[str]:
    """Check which requirements would be installed if the requirements file were installed.

    This function performs a dry-run pip install of the requirements and analyzes the output
    to determine which packages would be installed.

    Args:
        requirements (Path): Path to the requirements.txt file to check
        python (Path | None): Path to the Python interpreter to use. If None, uses current interpreter

    Returns:
        List[str]: A list of package specifications that would be installed, in the format
                  "package==version"

    Raises:
        RuntimeError: If the pip report format is not version 1
        subprocess.CalledProcessError: If the pip dry-run fails
    """

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
) -> None:
    """Set up a Python virtual environment with pip.
    This function creates a new virtual environment at the specified path and ensures pip is available
    within it. If pip is not available, it will attempt to bootstrap it through various methods.
    Args:
        output (Path): Path where the virtual environment should be created
        allow_fallback (bool): Whether to allow downloading pip from PyPA as a fallback
        system_site_packages (bool, optional): Whether the venv should have access to system packages.
            Defaults to False.
    Raises:
        RuntimeError: If pip cannot be bootstrapped and fallback is not allowed
    Returns:
        None
    Notes:
        The function tries the following methods to ensure pip availability, in order:
        1. Use system pip if system_site_packages is True and pip is installed
        2. Check if pip is already available in the new venv
        3. Use ensurepip module if available
        4. Download pip from PyPA if fallback is allowed
    """

    venv.create(output, with_pip=False, system_site_packages=system_site_packages)
    venv_python = output / "bin" / "python"

    if system_site_packages and HAS_PIP:
        # Venv can access the system's copy of Pip
        pass
    elif subprocess.run([venv_python, "-c", "import pip"], check=False).returncode == 0:
        pass
    elif HAS_ENSUREPIP:
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


def main() -> None:
    "Main function"

    args = parse_arguments()

    # Determine if the requirements are already satisfied on the system
    if args.skip_if_satisfied and HAS_PIP:
        to_install = check_missing_requirements(args.requirements, None)
        if not to_install:
            print("Requirements are already satisfied, skipping venv creation")
            sys.exit(7)
        print(
            f"Requirements not yet satisfied, would install: {', '.join(to_install)}",
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
            f"Requirements not yet satisfied, would install: {', '.join(to_install)}",
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
