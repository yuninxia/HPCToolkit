// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE

#include <alloca.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <stdbool.h>

#include <pthread.h>

#include <dlfcn.h>



//******************************************************************************
// libmonitor
//******************************************************************************

#include "../libmonitor/monitor.h"



//******************************************************************************
// local includes
//******************************************************************************

#include "simple_oo.h"
#include "sample_source_obj.h"
#include "common.h"

#include "../audit/audit-api.h"
#include "../control-knob.h"
#include "../device-finalizers.h"
#include "../logical/common.h"
#include "../gpu/activity/gpu-activity.h"
#include "../gpu/api/common/gpu-kernel-table.h"
#include "../gpu/api/intel/level0/level0-api.h"
#include "../gpu/common/gpu-monitoring.h"
#include "../gpu/gpu-metrics.h"
#include "../gpu/trace/gpu-trace-api.h"
#include "../gpu/api/common/gpu-instrumentation.h"
#include "../hpcrun_options.h"
#include "../hpcrun_stats.h"
#include "../metrics.h"
#include "../module-ignore-map.h"
#include "../ompt/ompt-interface.h"
#include "../safe-sampling.h"
#include "../sample_sources_registered.h"
#include "../sample_event.h"
#include "../thread_data.h"
#include "../trace.h"

#include "../utilities/tokenize.h"
#include "../messages/messages.h"
#include "../../common/lean/hpcrun-fmt.h"

#include "display.h"



//******************************************************************************
// macros
//******************************************************************************

#define INTEL_LEVEL0 "gpu=level0"
#define INTEL_LEVEL0_PC_SAMPLING "gpu=level0,pc"

// PC sample one of every 2^20 ns
#define LEVEL0_PC_SAMPLING_PERIOD_LOG_DEFAULT 20



//******************************************************************************
// local variables
//******************************************************************************

static device_finalizer_fn_entry_t device_finalizer_flush;
static device_finalizer_fn_entry_t device_finalizer_shutdown;
//static device_finalizer_fn_entry_t device_finalizer_trace;

static char event_name[128];

static gpu_instrumentation_t level0_instrumentation_options;


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
  TMSG(LEVEL0, "thread_init");
}


static void
METHOD_FN(thread_init_action)
{
  TMSG(LEVEL0, "thread_init_action");
}

static void
METHOD_FN(start)
{
  TMSG(LEVEL0, "start");
  TD_GET(ss_state)[self->sel_idx] = START;
}


static void
METHOD_FN(thread_fini_action)
{
  TMSG(LEVEL0, "thread_fini_action");
}


static void
METHOD_FN(stop)
{
  TMSG(LEVEL0, "stop");
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
  if (hpcrun_ev_is(ev_str, INTEL_LEVEL0) ||
      hpcrun_ev_is(ev_str, INTEL_LEVEL0_PC_SAMPLING)) {
    return true;
  }

#ifdef ENABLE_GTPIN
  return strncmp(ev_str, INTEL_LEVEL0,
                 strlen(INTEL_LEVEL0)) == 0;
#else
  return false;
#endif
}

static void
METHOD_FN(process_event_list)
{
  hpcrun_set_trace_metric(HPCRUN_GPU_TRACE_FLAG);

  char* evlist = METHOD_CALL(self, get_event_str);
  char* event = start_tok(evlist);
  long int value = 0;
  hpcrun_extract_ev_thresh(event, sizeof(event_name), event_name,
    &value, GPU_SAMPLING_PERIOD_UNSPECIFIED);

  bool pc_sampling_enabled = false;
  if (hpcrun_ev_is(event, INTEL_LEVEL0_PC_SAMPLING)) {
    pc_sampling_enabled = true;

    long sampling_period_log;

    if (value != GPU_SAMPLING_PERIOD_UNSPECIFIED) {
      sampling_period_log = value;
    } else {
      sampling_period_log = LEVEL0_PC_SAMPLING_PERIOD_LOG_DEFAULT;
    }

    gpu_monitoring_instruction_sampling_period_set(1 << sampling_period_log);

    gpu_metrics_GPU_CYCLES_TYPE_enable();
    gpu_metrics_GPU_INST_enable(); // instruction counts
    gpu_metrics_GPU_INST_TYPE_enable();
    gpu_metrics_GPU_INST_STALL_enable();
  }

  // Store PC sampling state for later use in level0_init
  level0_instrumentation_options.pc_sampling = pc_sampling_enabled;

  gpu_instrumentation_options_set(event_name, INTEL_LEVEL0, &level0_instrumentation_options);
  if (gpu_instrumentation_enabled(&level0_instrumentation_options)) {
     gpu_metrics_GPU_INST_enable();
  }

  gpu_metrics_default_enable();
  gpu_metrics_KINFO_enable();
}

static void
METHOD_FN(finalize_event_list)
{
  level0_init(&level0_instrumentation_options);

  // Init records
  gpu_trace_init();

  device_finalizer_flush.fn = level0_flush;
  device_finalizer_register(device_finalizer_type_flush, &device_finalizer_flush);

  device_finalizer_shutdown.fn = level0_fini;
  device_finalizer_register(device_finalizer_type_shutdown, &device_finalizer_shutdown);
}


static void
METHOD_FN(gen_event_set)
{
}


static void
METHOD_FN(display_events)
{
  display_header(stdout, "Available events for monitoring GPU operations atop Intel's Level Zero");
  display_header_event(stdout);

  display_event_info(stdout, "gpu=level0",
         "Operation-level monitoring for GPU-accelerated applications "
         "running atop Intel's Level Zero runtime. Collect timing "
         "information for GPU kernel invocations, memory copies, etc.");

  display_event_info(stdout, "gpu=level0,pc[@k]",
        "Comprehensive monitoring of operations on an Intel GPU as described above "
         "with the addition of PC sampling. "
         "The sample period will be 2^k ns. [Default: k=19]. Intel's hardware support "
         "for PC sampling profiles instruction issues, stalls, and stall reasons.");
#ifdef ENABLE_GTPIN
  display_event_info(stdout, "gpu=level0,inst=<comma-separated list of options>",
         "Operation-level monitoring for GPU-accelerated applications "
         "running atop Intel's Level Zero runtime. Collect timing "
         "information for GPU kernel invocations, memory copies, etc. "
         "When running on Intel GPUs, use optional instrumentation "
         "within GPU kernels to collect one or more of the following:\n"
         "count:   count how many times each GPU instruction executes\n"
#if ENABLE_LATENCY_ANALYSIS
         "latency: approximately attribute latency to GPU instructions\n"
#endif
#if ENABLE_SIMD_ANALYSIS
         "simd:    analyze utilization of SIMD lanes\n"
#endif
         "silent:  silence warnings from instrumentation");
#endif
}



//**************************************************************************
// object
//**************************************************************************

#define ss_name level0
#define ss_cls SS_HARDWARE
#define ss_sort_order  21

#include "ss_obj.h"
