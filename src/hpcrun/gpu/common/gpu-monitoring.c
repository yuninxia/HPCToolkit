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

//  #include "../messages/messages.h"
#include "gpu-monitoring.h"



//******************************************************************************
// system includes
//******************************************************************************

#include <string.h>
#include <stdio.h>
#include <stdlib.h>



//******************************************************************************
// macros
//******************************************************************************

#define DEBUG 0

#define GPU_INSTRUCTION_SAMPLING_PREFIX       "pc"
#define GPU_INSTRUCTION_SAMPLING_HOST         "sw"
#define GPU_INSTRUCTION_SAMPLING_GPU          "hw"
#define GPU_INSTRUCTION_SAMPLING_SERIALIZED   "serialized"



//******************************************************************************
// local data
//******************************************************************************

static const char *delimiter = ",";

static int gpu_trace_sample_period = -1;

gpu_monitoring_instruction_sampling_options_t instruction_sampling_options = {
  false, false, false, false, GPU_SAMPLING_PERIOD_UNSPECIFIED
};



//******************************************************************************
// private operations
//******************************************************************************

const char *bool_to_str(bool val)
{
  return (val ? "true" : "false");
}

//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_monitoring_instruction_sampling_period_set
(
  gpu_instruction_period_t sampling_period
)
{
  instruction_sampling_options.sampling_period = sampling_period;
}

gpu_instruction_period_t
gpu_monitoring_instruction_sampling_period_get
(
 void
)
{
  return instruction_sampling_options.sampling_period;
}


void
gpu_monitoring_trace_sampling_period_set
(
 uint32_t trace_sampling_period
)
{
  gpu_trace_sample_period = trace_sampling_period;
}


uint32_t
gpu_monitoring_trace_sample_period_get
(
 void
)
{
  return gpu_trace_sample_period;
}


void
gpu_monitoring_instruction_sampling_flags_set
(
 const char *arg_string,
 const char *arg_prefix
)
{
  char *opt = strdup(arg_string); // writable copy of arg_string
  char *ostr = 0;

  int len = strlen(opt);
  int match_len = strlen(arg_prefix);

  if (len > match_len) {
    ostr = opt + match_len;

    // match comma separator
    if (*ostr != ',') {
      fprintf(stderr, "ERROR: hpcrun: while parsing GPU PC sampling knobs, expected ',' separator but found '%s'\n", ostr);
      exit(-1);
    }
    ostr++;

    // match instrumentation prefix
    match_len = strlen(GPU_INSTRUCTION_SAMPLING_PREFIX);
    if (strncmp(ostr, GPU_INSTRUCTION_SAMPLING_PREFIX, match_len) != 0) {
      fprintf(stderr, "ERROR: hpcrun: while parsing GPU PC sampling knobs, expected 'pc' but found '%s'\n", ostr);
      exit(-1);
    }
    ostr += match_len;

    switch(*ostr) {
    case 0:
      // default instrumentation
      instruction_sampling_options.unspecified = true;
      break;

    case '=':
      ostr++;
      char *token = strtok(ostr, delimiter);

      // analyze options
      while(token) {
        if (strcmp(token, GPU_INSTRUCTION_SAMPLING_HOST) == 0) {
          instruction_sampling_options.software = true;
        } else if (strcmp(token, GPU_INSTRUCTION_SAMPLING_GPU) == 0) {
          instruction_sampling_options.hardware = true;
        } else if (strcmp(token, GPU_INSTRUCTION_SAMPLING_SERIALIZED) == 0) {
          instruction_sampling_options.serialized = true;
        } else {
          fprintf(stderr, "ERROR: hpcrun: while parsing GPU PC sampling knobs, unrecognized knob '%s'\n", token);
          exit(-1);
        }
        token = strtok(NULL, delimiter);
      }
      break;

    default:
      fprintf(stderr, "ERROR: hpcrun: unexpected text encountered parsing GPU PC sampling knobs '%s'\n", ostr);
      exit(-1);
    }
  }

  if (instruction_sampling_options.hardware && instruction_sampling_options.software) {
    fprintf(stderr, "ERROR: hpcrun: can't select both hardware (hw) and software (sw) PC sampling '%s'\n", arg_string);
    exit(-1);
  }

#if DEBUG
printf("gpu PC sampling options  : %s\n", opt);
  printf("\toptions.host         : %s\n", bool_to_str(options->host));
  printf("\toptions.gpu          : %s\n", bool_to_str(options->gpu));
  printf("\toptions.unspecified  : %s\n", bool_to_str(options->unspecified));
  printf("\toptions.serialized   : %s\n", bool_to_str(options->serialized));
#endif

  free(opt);
}


bool
gpu_monitoring_instruction_sampling_is_enabled
(
 unsigned int flags
)
{
  bool result = false;

  gpu_monitoring_instruction_sampling_options_t *options = &instruction_sampling_options;

  // incorporate options settings into result based on flag settings
  result |= (flags & GPU_INSTRUCTION_SAMPLING_SW) ? options->software : false;
  result |= (flags & GPU_INSTRUCTION_SAMPLING_HW) ? options->hardware : false;
  result |= (flags & GPU_INSTRUCTION_SAMPLING_UNSPECIFIED) ? options->unspecified : false;

  return result;
}
