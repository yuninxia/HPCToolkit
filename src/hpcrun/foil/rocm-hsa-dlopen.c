// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include "rocm-hsa-private.h"

const struct hpcrun_foil_appdispatch_rocm_hsa hpcrun_dispatch_rocm_hsa = {
    .hsa_init = &hsa_init,
    .hsa_system_get_info = &hsa_system_get_info,
};
