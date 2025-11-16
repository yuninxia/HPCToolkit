// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_monitoring_h
#define gpu_monitoring_h

#if defined(__cplusplus)
extern "C" {
#endif


//******************************************************************************
// system includes
//******************************************************************************

#include <stdbool.h>
#include <stdint.h>



//******************************************************************************
// macros
//******************************************************************************

#define GPU_INSTRUCTION_SAMPLING_SW          (1 << 0)
#define GPU_INSTRUCTION_SAMPLING_HW          (1 << 1)
#define GPU_INSTRUCTION_SAMPLING_UNSPECIFIED (1 << 2)

#define GPU_INSTRUCTION_SAMPLING_ANY \
  (GPU_INSTRUCTION_SAMPLING_SW | \
   GPU_INSTRUCTION_SAMPLING_HW | \
   GPU_INSTRUCTION_SAMPLING_UNSPECIFIED)

#define GPU_SAMPLING_PERIOD_UNSPECIFIED -1



//******************************************************************************
// type declarations
//******************************************************************************

typedef uint32_t gpu_instruction_sampling_period_t;

typedef struct gpu_monitoring_instruction_sampling_options_t {
  bool software;
  bool hardware;
  bool unspecified;
  bool serialized;
  gpu_instruction_sampling_period_t sampling_period;
} gpu_monitoring_instruction_sampling_options_t;


//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_monitoring_trace_sampling_period_set
(
 uint32_t trace_sampling_period
);


uint32_t
gpu_monitoring_trace_sampling_period_get
(
 void
);


void
gpu_monitoring_instruction_sampling_flags_set
(
  const char *arg_string,
  const char *arg_prefix
);


void
gpu_monitoring_instruction_sampling_period_set
(
  gpu_instruction_sampling_period_t sampling_period
);


bool
gpu_monitoring_instruction_sampling_is_enabled
(
  unsigned int flags
);


gpu_instruction_sampling_period_t
gpu_monitoring_instruction_sampling_period_get
(
 void
);

#if defined(__cplusplus)
}
#endif

#endif
