// SPDX-FileCopyrightText: 2019-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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
// macros
//******************************************************************************

#define AMD_ROCM "gpu=rocm"
#define AMD_ROCM_PC_SAMPLING "gpu=rocm,pc"
#define ROCM_CTR_PREFIX "rocm::"



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

// -1: disabled, otherwise period
static long pc_sampling_period = -1;
static long pc_sampling_period_default = 10000;

gpu_counter_set_t *rocm_counter_names = 0;



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
  bool is_rocm_counter = strncmp(ev_str, ROCM_CTR_PREFIX, strlen(ROCM_CTR_PREFIX)) == 0;
  return hpcrun_ev_is(ev_str, AMD_ROCM) ||
    hpcrun_ev_is(ev_str, AMD_ROCM_PC_SAMPLING) || is_rocm_counter;
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
  long int period = 0;
  int period_default = -1;

  bool pc_sampling_requested = false;
  bool hardware_counters_requested = false;

  for (; event != NULL; event = next_tok()) {
    static char rocm_event_name[128]; // buffer for parsing event name
    hpcrun_extract_ev_thresh(event, sizeof(rocm_event_name), rocm_event_name,
        &period, period_default);
    if (hpcrun_ev_is(event, AMD_ROCM)) {
      trace_period =
          (period == period_default) ? trace_period_default : period;
      gpu_monitoring_trace_sample_period_set(trace_period);
    } else if (hpcrun_ev_is(event, AMD_ROCM_PC_SAMPLING)) {
      pc_sampling_requested = true;

      pc_sampling_period =
          (period == period_default) ? pc_sampling_period_default : period;

      gpu_monitoring_instruction_sample_period_set(pc_sampling_period);

      gpu_metrics_GPU_INST_enable(); // instruction counts

      gpu_metrics_GPU_INST_STALL_enable(); // stall metrics
    } else {
      if (strncmp(rocm_event_name, ROCM_CTR_PREFIX, strlen(ROCM_CTR_PREFIX)) == 0) {
        hardware_counters_requested = true;
        gpu_counter_set_insert(rocm_counter_names, rocm_event_name + strlen(ROCM_CTR_PREFIX));
      }
    }
  }

  if (hardware_counters_requested && pc_sampling_requested) {
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
  display_header(stdout, "Available events for monitoring ROCM on AMD GPUs");

  display_header_event(stdout);

  display_event_info(stdout, AMD_ROCM,
    "Comprehensive operation-level monitoring on an AMD GPU. "
    "Collect timing information on GPU kernel invocations, "
    "memory copies, etc..");

  display_event_info(stdout, AMD_ROCM_PC_SAMPLING,
    "Comprehensive operation-level monitoring on an AMD GPU "
    "as described above with the addition of PC sampling. "
    "On some AMD GPUs, hardware support for PC sampling "
    "attributes STALL reasons to individual GPU instructions.");

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
