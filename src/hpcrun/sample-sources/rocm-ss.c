// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../device-finalizers.h"
#include "../gpu/api/rocm/rocm-interface.h"
#include "../gpu/api/rocm/rocm-counters.h"
#include "../gpu/common/gpu-monitoring.h"
#include "../gpu/common/gpu-counter-set.h"
#include "../gpu/gpu-metrics.h"
#include "../gpu/trace/gpu-trace-api.h"
#include "../messages/messages.h"
#include "../thread_data.h"
#include "../utilities/tokenize.h"

#include "common.h"
#include "rocm-ss.h"
#include "sample_source_obj.h"
#include "display.h"



//******************************************************************************
// system includes
//******************************************************************************

#include <stdio.h>    // Required for fprintf
#include <stdbool.h>  // Required for bool
#include <unistd.h>   // Required for access() function and F_OK constant



//******************************************************************************
// macros
//******************************************************************************

#define AMD_ROCM "gpu=rocm"
#define AMD_ROCM_GPU_INSTRUCTION_SAMPLING "gpu=rocm,pc"
#define ROCM_CTR_PREFIX "rocm::"

// minimum period = 256; sample one of every 2^20 instructions
#define ROCM_PC_SAMPLING_STOCHASTIC_PERIOD_LOG_DEFAULT_INST 20

// minimum period 1us; one out of every X unit time periods (ns), for
// the value of X specified below
#define ROCM_PC_SAMPLING_HOST_TRAP_PERIOD_LOG_DEFAULT_NS 17

#define MINIMUM_PC_SAMPLING_PERIOD 10

// The path KFD_TOPOLOGY_PATH is associated with the
// Linux Kernel Fusion Driver (KFD) used by AMD GPUs.
#define KFD_TOPOLOGY_PATH "/sys/class/kfd/kfd/topology/nodes"



//******************************************************************************
// private variables
//******************************************************************************

static bool rocm_monitoring_requested = false;

static device_finalizer_fn_entry_t device_finalizer_flush;
static device_finalizer_fn_entry_t device_finalizer_shutdown;
static device_finalizer_fn_entry_t device_trace_finalizer_shutdown;

// default trace all activities
// -1: disabled, >0: x ms per activity
static long trace_period = -1;
static long trace_period_default = -1;

gpu_counter_set_t *rocm_counter_names = 0;



//******************************************************************************
// private operations
//******************************************************************************

/// @brief  Check if KFD_TOPOLOGY_PATH needed for AMD GPUs exists in the filesystem.
/// @return bool: true if KFD_TOPOLOGY_PATH exists, false otherwise.
static bool
check_kfd_topology_dir_exists()
{
  // access() checks file/directory permissions or existence.
  // F_OK mode checks if the file exists.
  // It returns 0 on success (path exists) and -1 on failure
  // (path does not exist or permission denied).
  return (access(KFD_TOPOLOGY_PATH, F_OK) == 0);
}



//******************************************************************************
// interface operations
//******************************************************************************

static void
METHOD_FN(init)
{
  self->state = INIT;
}


static void
METHOD_FN(thread_init)
{
  TMSG(ROCM, "thread_init");
}


static void
METHOD_FN(thread_init_action)
{
  TMSG(ROCM, "thread_init_action");
}


static void
METHOD_FN(start)
{
  TMSG(ROCM, "start");
  TD_GET(ss_state)[self->sel_idx] = START;
}


static void
METHOD_FN(thread_fini_action)
{
  TMSG(ROCM, "thread_fini_action");
}


static void
METHOD_FN(stop)
{
  hpcrun_get_thread_data();

  TD_GET(ss_state)[self->sel_idx] = STOP;
}


static void
METHOD_FN(shutdown)
{
  self->state = UNINIT;
}


static bool
METHOD_FN(supports_event, const char *ev_str)
{
  // only support rocm events if AMD GPU hardware is present
  if (check_kfd_topology_dir_exists() == false) return false;

  bool is_rocm_counter = strncmp(ev_str, ROCM_CTR_PREFIX, strlen(ROCM_CTR_PREFIX)) == 0;
  return (strncmp(ev_str, AMD_ROCM, strlen(AMD_ROCM)) == 0) || is_rocm_counter;
}


static void
METHOD_FN(process_event_list)
{
  int nevents = (self->evl).nevents;
  TMSG(ROCM, "nevents = %d", nevents);

  rocm_monitoring_requested = true;

  rocm_counter_names = gpu_counter_set_new();

  hpcrun_set_trace_metric(HPCRUN_GPU_TRACE_FLAG);

  // Fetch the event string for the sample source
  // only one event is allowed
  char* evlist = METHOD_CALL(self, get_event_str);
  char* event = start_tok(evlist);
  long int value = 0;
  int period_default = GPU_SAMPLING_PERIOD_UNSPECIFIED;

  bool instruction_sampling_requested = false;
  bool hardware_counters_requested = false;

  for (; event != NULL; event = next_tok()) {
    static char rocm_event_name[128]; // buffer for parsing event name
    hpcrun_extract_ev_thresh(event, sizeof(rocm_event_name), rocm_event_name,
        &value, period_default);
    if (hpcrun_ev_is(event, AMD_ROCM)) {
      trace_period =
          (value == GPU_SAMPLING_PERIOD_UNSPECIFIED) ? trace_period_default : value;
      gpu_monitoring_trace_sampling_period_set(trace_period);
    } else if (strncmp(event, AMD_ROCM_GPU_INSTRUCTION_SAMPLING,
                       strlen(AMD_ROCM_GPU_INSTRUCTION_SAMPLING)) == 0) {
      // attempt to set flags and period
      gpu_monitoring_instruction_sampling_flags_set(event, AMD_ROCM);

      // if PC sampling of any kind is enabled
      if (gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_ANY)) {
        instruction_sampling_requested = true;

        // NOTE: rocprofiler-sdk stochastic PC sampling requires sampling periods to be a power
        // of two. enforce that for host trap sampling so that periods are always specified
        // in a consistent fashion.

        // for stochastic sampling, the period unit is GPU cycles (minimum = 256)
        // for host-trap sampling, the unit is an unspecified time unit (minimum = 1)

        long sampling_period_log;

        if (value != GPU_SAMPLING_PERIOD_UNSPECIFIED) {
          sampling_period_log = 
            (value > MINIMUM_PC_SAMPLING_PERIOD) ? value : MINIMUM_PC_SAMPLING_PERIOD;
        } else {
          sampling_period_log =
            gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_HW) ?
            ROCM_PC_SAMPLING_STOCHASTIC_PERIOD_LOG_DEFAULT_INST :
            ROCM_PC_SAMPLING_HOST_TRAP_PERIOD_LOG_DEFAULT_NS;
        }

        gpu_monitoring_instruction_sampling_period_set(1 << sampling_period_log);

        gpu_metrics_GPU_INST_enable(); // instruction counts
        gpu_metrics_GPU_INST_TYPE_enable(); // instruction type metrics
        if (gpu_monitoring_instruction_sampling_is_enabled(GPU_INSTRUCTION_SAMPLING_HW)) {
          // only enable stall metrics if using HW support for PC sampling.
          // they are unavailable with host_trap SW support.
          gpu_metrics_GPU_INST_STALL_enable(); // stall metrics
          gpu_metrics_GPU_PIPE_enable(); // pipeline utilization metrics
          gpu_metrics_GPU_UTIL_METRICS_enable(); // wave and SIMD utilization
        }
      }
    } else if (strncmp(rocm_event_name, ROCM_CTR_PREFIX, strlen(ROCM_CTR_PREFIX)) == 0) {
        hardware_counters_requested = true;
        gpu_counter_set_insert(rocm_counter_names, rocm_event_name + strlen(ROCM_CTR_PREFIX));
    } else {
      fprintf(stderr, "ERROR: hpcrun: unrecognized suffix in ROCm monitoring specification '%s'\n",
        rocm_event_name + strlen(AMD_ROCM));
      exit(-1);
    }
  }

  if (hardware_counters_requested && instruction_sampling_requested) {
    fprintf(stderr, "ERROR: "
      "hpcrun: rocprofiler-sdk does not support use of PC sampling "
      "and hardware counters in the same execution\n");
    exit(1);
  }

  gpu_metrics_default_enable();

  gpu_metrics_KINFO_enable();
}


static void
METHOD_FN(finalize_event_list)
{

  if (rocm_monitoring_requested == false) return;

  rocm_interface_init(rocm_counter_names);

  device_finalizer_flush.fn = rocm_interface_flush;
  device_finalizer_register(device_finalizer_type_flush,
    &device_finalizer_flush);

  device_finalizer_shutdown.fn = rocm_interface_fini;
  device_finalizer_register(device_finalizer_type_shutdown,
    &device_finalizer_shutdown);

  // initialize gpu tracing
  gpu_trace_init();

  // Register shutdown function to finalize gpu tracing and write trace files
  device_trace_finalizer_shutdown.fn = gpu_trace_fini;
  device_finalizer_register(device_finalizer_type_shutdown,
    &device_trace_finalizer_shutdown);
}


static void
METHOD_FN(gen_event_set)
{
}


static void
METHOD_FN(display_events)
{
  if (check_kfd_topology_dir_exists() == false) {
    // no KFD driver, therefore no AMD GPU options are available
    return;
  }

  display_header(stdout, "Available events for monitoring ROCM on AMD GPUs");

  display_header_event(stdout);

  display_event_info(stdout, AMD_ROCM,
    "Comprehensive operation-level monitoring on an AMD GPU. "
    "Collect timing information on GPU kernel invocations, "
    "memory copies, etc..");

  display_event_info(stdout, AMD_ROCM_GPU_INSTRUCTION_SAMPLING "[={hw,sw}][@k]",
    "Comprehensive operation-level monitoring on an AMD GPU "
    "as described above with the addition of PC sampling. "
    "\n\nMI200+ GPUs may provide software ('sw') support for PC sampling, "
    "referred to by AMD as 'HOST_TRAP' sampling. If 'sw' sampling is "
    "selected, the period will be set to 2^k ns [Default k=17]. "
    "For 'sw' sampling, the minimum value for k=10. "
    "Software support for PC sampling will only profile instructions sampled. "
    "\n\nMI300+ GPUs may provide hardware ('hw') support for PC sampling, "
    "referred to by AMD as 'STOCHASTIC' sampling. If 'hw' sampling is "
    "selected, the period will be set to 2^k instructions [Default k=20]. "
    "The minimum value of k=8. (WARNING: small values of k cause HSA/ROCm errors.) "
    "Hardware support for PC sampling "
    "will profile instruction issues, non-issues, instruction types, "
    "issue stalls, pipeline issues, pipeline stalls, wave utilization, and "
    "thread utilization.");

  display_header(stdout, "Available hardware counters for monitoring GPU kernels on AMD GPUs");

  display_header_event(stdout);

  rocm_counters_list(display_event_info, ROCM_CTR_PREFIX, stdout);
}


//**************************************************************************
// object
//**************************************************************************

#define ss_name rocm_gpu
#define ss_cls SS_HARDWARE
#define ss_sort_order  23

#include "ss_obj.h"
