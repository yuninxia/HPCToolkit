// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//
// attribute GPU performance metrics
//

//******************************************************************************
// local includes
//******************************************************************************

#define _GNU_SOURCE

#include "gpu-monitoring.h"



//******************************************************************************
// local variables
//******************************************************************************

static int gpu_inst_sample_period = -1;

static int gpu_trace_sample_period = -1;



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_monitoring_instruction_sample_period_set
(
 uint32_t inst_sample_period
)
{
  gpu_inst_sample_period = inst_sample_period;
}


uint32_t
gpu_monitoring_instruction_sample_period_get
(
 void
)
{
  return gpu_inst_sample_period;
}


void
gpu_monitoring_trace_sample_period_set
(
 uint32_t trace_sample_period
)
{
  gpu_trace_sample_period = trace_sample_period;
}


uint32_t
gpu_monitoring_trace_sample_period_get
(
 void
)
{
  return gpu_trace_sample_period;
}
