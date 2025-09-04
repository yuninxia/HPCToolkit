#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

# pylint: disable=missing-module-docstring,invalid-name

import os
import shutil
import sys

# get absolute input and output paths
input_path = os.path.join(
    os.getenv("MESON_SOURCE_ROOT", ""), os.getenv("MESON_SUBDIR", ""), sys.argv[1]
)

output_path = os.path.join(
    os.getenv("MESON_BUILD_ROOT", ""), os.getenv("MESON_SUBDIR", ""), sys.argv[2]
)

# make sure destination directory exists
os.makedirs(os.path.dirname(output_path), exist_ok=True)

# and finally copy the file
shutil.copyfile(input_path, output_path)
