// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// hpctoolkit includes
//*****************************************************************************

#define _GNU_SOURCE

#include "level0-command-process.h"
#include "level0-correlation-channels.h"
#include "level0-data-node.h"
#include "level0-binary.h"
#include "level0-api.h"

#include "../../../activity/gpu-activity.h"
#include "../../../activity/gpu-activity-channel.h"
#include "../../../activity/gpu-activity-process.h"
#include "../../../activity/gpu-activity-send.h"
#include "../../../activity/correlation/gpu-correlation-channel.h"
#include "../../../api/nvidia/cuda-correlation-id-map.h"
#include "../../../activity/correlation/gpu-host-correlation-map.h"
#include "../../../gpu-monitoring-thread-api.h"
#include "../../common/gpu-cid-map.h"
#include "../../../gpu-application-thread-api.h"
#include "../../common/gpu-kernel-table.h"

#include "../../common/gpu-cct.h"

#ifdef ENABLE_GTPIN
#include "../gtpin/gtpin-instrumentation.h"
#endif

#include "../../../../audit/audit-api.h"
#include "../../../../safe-sampling.h"
#include "../../../../logical/common.h"
#include "../../../../utilities/hpcrun-nanotime.h"
#include "../../../../../common/lean/crypto-hash.h"
#include <stdatomic.h>
#include "../../../../../common/lean/usec_time.h"

#include "../../../../libmonitor/monitor.h"

#include "../../../activity/correlation/gpu-channel-common.h"

#include <inttypes.h>



//*****************************************************************************
// debugging
//*****************************************************************************

#define DEBUG 0
#include "../../../common/gpu-print.h"



//*****************************************************************************
// local variables
//*****************************************************************************

// The thread itself how many pending operations
static __thread atomic_int level0_self_pending_operations = 0;

//*****************************************************************************
// private operations
//*****************************************************************************

static void
level0_kernel_translate
(
  gpu_activity_t* ga,
  level0_data_node_t* c,
  uint64_t start,
  uint64_t end
)
{
  PRINT("level0_kernel_translate: submit_time %lu, start %lu, end %lu --> tstart %lu tend %lu\n",
    c->submit_time, start, end,
    level0_timestamp_to_realtime(c->submit_time, start),
    level0_timestamp_to_realtime(c->submit_time, end));

  ga->kind = GPU_ACTIVITY_KERNEL;
  ga->details.kernel.kernel_first_pc = ip_normalized_NULL;
  ga->details.kernel.correlation_id = c->correlation_id;
  ga->details.kernel.submit_time = c->submit_time;
  gpu_interval_set(&ga->details.interval,
    level0_timestamp_to_realtime(c->submit_time, start),
    level0_timestamp_to_realtime(c->submit_time, end));
}

static void
level0_memcpy_translate
(
  gpu_activity_t* ga,
  level0_data_node_t* c,
  uint64_t start,
  uint64_t end
)
{
  PRINT("level0_memcpy_translate: src_type %d, dst_type %d, size %lu start %lu end %lu --> tstart %lu tend %lu\n",
    c->details.memcpy.src_type,
    c->details.memcpy.dst_type,
    c->details.memcpy.copy_size,
    start, end,
    level0_timestamp_to_realtime(c->submit_time, start),
    level0_timestamp_to_realtime(c->submit_time, end));

  ga->kind = GPU_ACTIVITY_MEMCPY;
  ga->details.memcpy.bytes = c->details.memcpy.copy_size;
  ga->details.memcpy.correlation_id = c->correlation_id;
  ga->details.memcpy.submit_time = c->submit_time;

  // Switch on memory src and dst types
  ga->details.memcpy.copyKind = GPU_MEMCPY_UNK;
  switch (c->details.memcpy.src_type) {
    case ZE_MEMORY_TYPE_HOST: {
      switch (c->details.memcpy.dst_type) {
        case ZE_MEMORY_TYPE_HOST:
          ga->details.memcpy.copyKind = GPU_MEMCPY_H2H;
          break;
        case ZE_MEMORY_TYPE_UNKNOWN:
        case ZE_MEMORY_TYPE_DEVICE:
          ga->details.memcpy.copyKind = GPU_MEMCPY_H2D;
          break;
        case ZE_MEMORY_TYPE_SHARED:
          ga->details.memcpy.copyKind = GPU_MEMCPY_H2A;
          break;
        default:
          break;
      }
      break;
    }
    case ZE_MEMORY_TYPE_DEVICE: {
      switch (c->details.memcpy.dst_type) {
        case ZE_MEMORY_TYPE_UNKNOWN:
        case ZE_MEMORY_TYPE_HOST:
          ga->details.memcpy.copyKind = GPU_MEMCPY_D2H;
          break;
        case ZE_MEMORY_TYPE_DEVICE:
          ga->details.memcpy.copyKind = GPU_MEMCPY_D2D;
          break;
        case ZE_MEMORY_TYPE_SHARED:
          ga->details.memcpy.copyKind = GPU_MEMCPY_D2A;
          break;
        default:
          break;
      }
      break;
    }
    case ZE_MEMORY_TYPE_SHARED: {
      switch (c->details.memcpy.dst_type) {
        case ZE_MEMORY_TYPE_UNKNOWN:
        case ZE_MEMORY_TYPE_HOST:
          ga->details.memcpy.copyKind = GPU_MEMCPY_A2H;
          break;
        case ZE_MEMORY_TYPE_DEVICE:
          ga->details.memcpy.copyKind = GPU_MEMCPY_A2D;
          break;
        case ZE_MEMORY_TYPE_SHARED:
          ga->details.memcpy.copyKind = GPU_MEMCPY_A2A;
          break;
        default:
          break;
      }
      break;
    }
    case ZE_MEMORY_TYPE_UNKNOWN: {
      switch (c->details.memcpy.dst_type) {
        case ZE_MEMORY_TYPE_HOST:
          ga->details.memcpy.copyKind = GPU_MEMCPY_D2H;
          break;
        case ZE_MEMORY_TYPE_DEVICE:
          ga->details.memcpy.copyKind = GPU_MEMCPY_H2D;
          break;
        case ZE_MEMORY_TYPE_SHARED:
          ga->details.memcpy.copyKind = GPU_MEMCPY_D2A;
          break;
        case ZE_MEMORY_TYPE_UNKNOWN:
          ga->details.memcpy.copyKind = GPU_MEMCPY_UNK;
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  gpu_interval_set(&ga->details.interval,
    level0_timestamp_to_realtime(c->submit_time, start),
    level0_timestamp_to_realtime(c->submit_time, end));
}


//*****************************************************************************
// interface operations
//*****************************************************************************


// Expand this function to create GPU side cct
void
level0_command_begin
(
  level0_data_node_t* command_node
)
{
  // Increment the operation counter for this thread
  atomic_fetch_add(&level0_self_pending_operations, 1);
  command_node->pending_operations = &level0_self_pending_operations;

  uint64_t correlation_id = gpu_activity_channel_generate_correlation_id();
  command_node->correlation_id = correlation_id;

  cct_node_t *api_node =
    gpu_application_thread_correlation_callback(correlation_id);

  hpcrun_safe_enter();

  ip_normalized_t kernel_ip = ip_normalized_NULL;

  if (command_node->type == LEVEL0_KERNEL) {

    ze_kernel_handle_t kernel = command_node->details.kernel.kernel;
    size_t name_size = 0;
    f_zeKernelGetName(kernel, &name_size, NULL, command_node->dispatch);
    char* kernel_name = malloc(name_size);
    f_zeKernelGetName(kernel, &name_size, kernel_name, command_node->dispatch);
#ifdef ENABLE_GTPIN
    if (level0_gtpin_enabled()) {
      kernel_ip = gtpin_lookup_kernel_ip(kernel_name);
    } else
#endif  // ENABLE_GTPIN
    {
      if (level0_metrics_requested()) {
        kernel_ip = level0_func_ip_resolve(kernel, command_node->dispatch);
      } else {
        kernel_ip = gpu_kernel_table_get(kernel_name, LOGICAL_MANGLING_CPP);
      }
    }
    free(kernel_name);
    command_node->kernel = api_node;
  }
  hpcrun_safe_exit();

  command_node->cct_node = api_node;
  gpu_cid_map_insert(correlation_id, api_node, kernel_ip);

  // Send correlation ID to PC sampling thread for kernel launches
  if (command_node->type == LEVEL0_KERNEL && level0_metrics_requested()) {
    gpu_activity_channel_t *channel = gpu_activity_channel_get_local();
    int32_t device_id = command_node->device_id;
    uint64_t channel_idx = level0CorrelationChannelIndex(device_id);
    if (channel_idx >= GPU_CHANNEL_TOTAL) {
      TMSG(LEVEL0, "Correlation channel index %" PRIu64 " exceeds limit %d; falling back to base channel",
           channel_idx, GPU_CHANNEL_TOTAL);
      channel_idx = LEVEL0_CORRELATION_CHANNEL_BASE;
    }
    gpu_correlation_channel_send(channel_idx, correlation_id, channel);
  }

  gpu_application_thread_process_activities();

  // Generate host side operation timestamp
  command_node->submit_time = hpcrun_nanotime();

#ifdef ENABLE_GTPIN
  if (command_node->type == LEVEL0_KERNEL && level0_gtpin_enabled()) {
    // Callback to produce gtpin correlation
    gtpin_produce_runtime_callstack(correlation_id);
  }
#endif
}

void
level0_command_end
(
  level0_data_node_t* command_node,
  uint64_t start,
  uint64_t end
)
{
  gpu_application_thread_process_activities();

  gpu_monitoring_thread_activities_ready();
  gpu_activity_t gpu_activity;
  gpu_activity_t* ga = &gpu_activity;
  memset(ga, 0, sizeof(gpu_activity_t));
  ga->cct_node = command_node->cct_node;
  PRINT("cct node %p, command node type %d\n", ga->cct_node, command_node->type);
  switch (command_node->type) {
    case LEVEL0_KERNEL:
      ga->cct_node = command_node->kernel;
      level0_kernel_translate(ga, command_node, start, end);
      break;
    case LEVEL0_MEMCPY:
      level0_memcpy_translate(ga, command_node, start, end);
      break;
    default:
      break;
  }

  cstack_ptr_set(&(ga->next), 0);

  atomic_fetch_add(&level0_self_pending_operations, -1);
  gpu_activity_send(command_node->correlation_id, ga);
}

void
level0_flush_and_wait
(
  void
)
{

}

void
level0_wait_for_self_pending_operations
(
  void
)
{
  struct timespec timeout;
  clock_gettime(CLOCK_MONOTONIC, &timeout);
  timeout.tv_sec += 10;
  while (atomic_load(&level0_self_pending_operations) != 0) {
    sched_yield();
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);
    if (current.tv_sec > timeout.tv_sec ||
        (current.tv_sec == timeout.tv_sec && current.tv_nsec >= timeout.tv_nsec)) {
      break;
    }
  }
}
