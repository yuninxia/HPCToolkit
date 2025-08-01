// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef WRITE_DATA_H
#define WRITE_DATA_H

#include "epoch.h"
#include "core_profile_trace_data.h"


extern int hpcrun_write_profile_data(core_profile_trace_data_t * cptd);
extern void hpcrun_flush_epochs(core_profile_trace_data_t * cptd);

#endif // WRITE_DATA_H
