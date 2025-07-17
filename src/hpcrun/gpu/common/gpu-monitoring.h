// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_monitoring_h
#define gpu_monitoring_h



//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_monitoring_instruction_sample_period_set
(
 uint32_t inst_sample_period
);


uint32_t
gpu_monitoring_instruction_sample_period_get
(
 void
);


void
gpu_monitoring_trace_sample_period_set
(
 uint32_t trace_sample_period
);


uint32_t
gpu_monitoring_trace_sample_period_get
(
 void
);



#endif
