#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

# pylint: disable=missing-module-docstring

import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("source_root", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("input", type=Path, nargs="+")
args = parser.parse_args()

with args.output.open("w", encoding="utf-8") as dstf:
    for srcfn in args.input:
        dstf.write(f"\n\n###FILE: {srcfn}\n\n")
        with srcfn.open(encoding="utf-8") as srcf:
            for line in srcf:
                line = line.rstrip()
                assert "%(cur_dir)s" not in line
                line = line.replace("%(xed_dir)s", str(args.source_root))
                dstf.write(line + "\n")
