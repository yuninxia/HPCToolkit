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




//******************************************************************************
// macros
//******************************************************************************

#define INTEL_LEVEL0 "gpu=level0"
#define INTEL_LEVEL0_PC_SAMPLING "gpu=level0,pc"

#define NO_THRESHOLD  1L



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
  return hpcrun_ev_is(ev_str, INTEL_LEVEL0) ||
         hpcrun_ev_is(ev_str, INTEL_LEVEL0_PC_SAMPLING);
}

static void
METHOD_FN(process_event_list)
{
  hpcrun_set_trace_metric(HPCRUN_GPU_TRACE_FLAG);
  gpu_metrics_default_enable();
  gpu_metrics_KINFO_enable();

  char* evlist = METHOD_CALL(self, get_event_str);
  char* event = start_tok(evlist);
  long th;
  hpcrun_extract_ev_thresh(event, sizeof(event_name), event_name,
    &th, NO_THRESHOLD);

  bool pc_sampling_enabled = false;
  if (hpcrun_ev_is(event, INTEL_LEVEL0_PC_SAMPLING)) {
    pc_sampling_enabled = true;

    // Intel Level Zero returns actual stall counts from hardware counters, not sample counts.
    // Setting sample_period to 0 means multiplier = 1 << 0 = 1, preserving the raw counts
    // when gpu-metrics.c:1203 calculates: stall_count = latencySamples * sample_period.
    // This differs from NVIDIA/AMD which return sample counts that need period-based scaling.
    gpu_monitoring_instruction_sample_period_set(0);

    gpu_metrics_GPU_INST_enable(); // instruction counts
    gpu_metrics_GPU_INST_STALL_enable();
  }

  // Store PC sampling state for later use in level0_init
  level0_instrumentation_options.pc_sampling = pc_sampling_enabled;

  gpu_instrumentation_options_set(event_name, INTEL_LEVEL0, &level0_instrumentation_options);
  if (gpu_instrumentation_enabled(&level0_instrumentation_options)) {
     gpu_metrics_GPU_INST_enable();
  }
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
  printf("===========================================================================\n");
  printf("Available events for monitoring GPU operations atop Intel's Level Zero \n");
  printf("===========================================================================\n");
  printf("Name\t\tDescription\n");
  printf("---------------------------------------------------------------------------\n");
  printf("gpu=level0\tOperation-level monitoring for GPU-accelerated applications\n"
         "\t\trunning atop Intel's Level Zero runtime. Collect timing \n"
         "\t\tinformation for GPU kernel invocations, memory copies, etc.\n"
         "\n");
  printf("gpu=level0,pc\tComprehensive monitoring on an Intel GPU as described above\n"
         "\t\twith the addition of PC sampling. PC sampling attributes\n"
         "\t\tSTALL reasons to individual GPU instructions and provides\n"
         "\t\tperformance counter data for GPU kernel execution analysis.\n"
         "\n");
#ifdef ENABLE_GTPIN
  printf("gpu=level0,inst=<comma-separated list of options>\n"
         "\t\tOperation-level monitoring for GPU-accelerated applications\n"
         "\t\trunning atop Intel's Level Zero runtime. Collect timing\n"
         "\t\tinformation for GPU kernel invocations, memory copies, etc.\n"
         "\t\tWhen running on Intel GPUs, use optional instrumentation\n"
         "\t\twithin GPU kernels to collect one or more of the following:\n"
         "\t\t  count:   count how many times each GPU instruction executes\n"
#if ENABLE_LATENCY_ANALYSIS
         "\t\t  latency: approximately attribute latency to GPU instructions\n"
#endif
#if ENABLE_SIMD_ANALYSIS
         "\t\t  simd:    analyze utilization of SIMD lanes\n"
#endif
         "\t\t  silent:  silence warnings from instrumentation\n"
         "\n");
#endif
}



//**************************************************************************
// object
//**************************************************************************

#define ss_name level0
#define ss_cls SS_HARDWARE
#define ss_sort_order  21

#include "ss_obj.h"
