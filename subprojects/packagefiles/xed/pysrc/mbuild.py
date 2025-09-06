# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: MIT

# pylint: disable=missing-module-docstring,invalid-name,missing-function-docstring

# XED uses a few functions from mbuild as part of the code generation process.
# This file provides short alternative implementations to make the build happy.

import re
import sys
import traceback
from pathlib import Path


def removesuffix(s, suff):
    return s[: -len(suff)] if s.endswith(suff) else s


def join(*args):
    return "/".join(
        [re.sub(r"""^(['"])(.*)\1$""", r"\2", removesuffix(a, "/")) for a in args]
    )


def msg(s, pad=""):
    print(pad + s)


def msgb(s, t="", pad=""):
    msg(f"[{s}] {t}", pad=pad)


def remove_file(fn, env=None, quiet=True):  # pylint: disable=unused-argument
    Path(fn).unlink(missing_ok=True)
    return 0, []


def cmkdir(path_to_dir):
    Path(path_to_dir).mkdir(parents=True, exist_ok=True)


def die(m, s=""):
    msgb("MBUILD ERROR", f"{m} {s}\n\n")
    if sys.exc_info() != (None, None, None):
        traceback.print_exc(file=sys.stdout)
    else:
        traceback.print_stack(file=sys.stdout)
    sys.exit(1)
