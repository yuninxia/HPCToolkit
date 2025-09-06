#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

# pylint: disable=missing-module-docstring,invalid-name

import argparse
import shutil
import subprocess
import sys
from pathlib import Path, PurePath

# import os

parser = argparse.ArgumentParser()
parser.add_argument("generator_dir", type=Path)
parser.add_argument("output_dir", type=Path)
parser.add_argument("--outputs", nargs="*", type=PurePath)
parser.add_argument("command", nargs="+")
args = parser.parse_args()

# pysrc_dir = Path(sys.argv[0]).resolve(strict=True).parent / "pysrc"
# if os.environ.get("PYTHONPATH"):
#     os.environ["PYTHONPATH"] += ":" + pysrc_dir
# else:
#     os.environ["PYTHONPATH"] = str(pysrc_dir)

# print(os.environ["PYTHONPATH"])

if args.generator_dir.exists():
    shutil.rmtree(args.generator_dir, ignore_errors=True)
args.generator_dir.mkdir(parents=True)
(args.generator_dir / "include-private").mkdir()
with (args.generator_dir / "DEC-OUT.txt").open("w", encoding="utf-8") as outf:
    subprocess.run([sys.executable] + args.command, check=True, stdout=outf)

topdirs = (args.generator_dir, args.generator_dir / "include-private")
output_names = {out.name for out in args.outputs}

for generated in args.generator_dir.rglob("*"):
    if generated.parent not in topdirs:
        raise ValueError(
            f"Generator produced a file in an unexpected subdirectory: {generated}"
        )

for gendir in topdirs:
    for generated in gendir.glob("*.[hc]"):
        if generated.name not in output_names:
            raise ValueError(f"Generator produced unexpected output: {generated}")
        shutil.copyfile(generated, args.output_dir / generated.name)
