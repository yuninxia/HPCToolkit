// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include "rocm-hip-private.h"

const struct hpcrun_foil_appdispatch_rocm_hip hpcrun_dispatch_rocm_hip = {
    .hipDeviceSynchronize = &hipDeviceSynchronize,
    .hipDeviceGetAttribute = &hipDeviceGetAttribute,
    .hipStreamSynchronize = &hipStreamSynchronize,
};
