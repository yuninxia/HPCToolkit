// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#define _GNU_SOURCE

#include "../ompt/ompt-interface.h"

#define BLAME_NAME OMP_IDLE
#define BLAME_LAYER OpenMP
#define BLAME_REQUEST ompt_idle_blame_shift_request

#include "blame-shift/blame-sample-source.h"
