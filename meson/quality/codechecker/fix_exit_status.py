#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

"Run CodeChecker but ignore exit status 2 (there was at least 1 report)"

import subprocess
import sys

if __name__ == "__main__":
    result = subprocess.run(sys.argv[1:], check=False)
    sys.exit(0 if result.returncode in (0, 2) else result.returncode)
