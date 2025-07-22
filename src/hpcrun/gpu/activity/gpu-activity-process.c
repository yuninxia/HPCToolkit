// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE

#include <assert.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "../../cct/cct.h"
#include "../../hpcrun_stats.h"
#include "../../messages/messages.h"

#include "gpu-op-ccts-map.h"
#include "../trace/gpu-trace-api.h"
#include "../trace/gpu-trace-item.h"
#include "../api/common/gpu-cid-map.h"

#include "gpu-activity.h"
#include "gpu-activity-process.h"
#include "gpu-event-id-map.h"


//******************************************************************************
// macros
//******************************************************************************


//  2021-09-08:
//  The timestamps for GPU synchronziation operations are currently
//  taken when entering and exiting synchronization APIs on the host.
//  Such timestamps are useless for GPU monitoring.
//  This is the case for NVIDIA and AMD.
//  We should revisit this if we start to get GPU side time stamps
//  regarding inter-stream synchronization.
#define TRACK_SYNCHRONIZATION 0

#define DEBUG 0

#include "../common/gpu-print.h"

#define IGNORE_IF_INVALID_INTERVAL(gi) \
  if (gpu_interval_is_invalid((gpu_interval_t *) &(gi->details))) return



//******************************************************************************
// private operations
//******************************************************************************

static cct_node_t *
get_placeholder_node
(
  uint64_t correlation_id,
  gpu_placeholder_type_t pht,
  ip_normalized_t *kernel_ip
)
{
  gpu_cid_map_info_t *info = gpu_cid_map_find(correlation_id);
  assert(info);

  cct_node_t *ph = hpcrun_cct_insert_ip_norm(info->node, gpu_op_placeholder_ip(pht), true);

  *kernel_ip = info->kernel_ip;

  return ph;
}

static void
gpu_context_stream_trace
(
 uint32_t device_id,
 uint32_t context_id,
 uint32_t stream_id,
 gpu_trace_item_t *ti
)
{
  gpu_trace_process_stream(device_id, context_id, stream_id, ti);
}


static void
trace_item_set
(
 gpu_trace_item_t *ti,
 gpu_activity_t *ga,
 uint64_t cpu_submit_time,
 cct_node_t *call_path_leaf
)
{
  gpu_trace_item_init(ti, cpu_submit_time, ga->details.interval.start,
                      ga->details.interval.end, call_path_leaf);
}


static void
gpu_memcpy_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.memcpy.correlation_id;

  gpu_placeholder_type_t mct;
  switch (activity->details.memcpy.copyKind) {
    case GPU_MEMCPY_H2D:
      mct = gpu_placeholder_type_copyin;
      break;
    case GPU_MEMCPY_D2H:
      mct = gpu_placeholder_type_copyout;
      break;
    default:
      mct = gpu_placeholder_type_copy;
      break;
  }

  ip_normalized_t dummy;
  cct_node_t *host_op_node = get_placeholder_node(correlation_id, mct, &dummy);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, activity->details.memcpy.start, host_op_node);

  gpu_context_stream_trace
    (activity->details.memcpy.device_id, activity->details.memcpy.context_id,
      activity->details.memcpy.stream_id, &entry_trace);
  activity->cct_node = host_op_node;
  //FIXME(keren): In OpenMP, an external_id may maps to multiple cct_nodes
  //gpu_host_correlation_map_delete(external_id);

  PRINT("Memcpy copy correlation_id 0x%lx\n", correlation_id);
  PRINT("Memcpy copy kind %u\n", activity->details.memcpy.copyKind);
  PRINT("Memcpy copy bytes %lu\n", activity->details.memcpy.bytes);
}


static void
gpu_unknown_process
(
 gpu_activity_t *activity
)
{
  PRINT("Unknown activity kind %d\n", activity->kind);
}


static void
gpu_sample_process
(
 gpu_activity_t* sample
)
{
  uint64_t correlation_id = sample->details.pc_sampling.correlation_id;

  ip_normalized_t ip = sample->details.pc_sampling.pc;

  gpu_placeholder_type_t pht =
    (ip.lm_id == 0) ? gpu_placeholder_type_kernel_anon : gpu_placeholder_type_kernel;

  ip_normalized_t dummy;
  cct_node_t *host_op_node = get_placeholder_node(correlation_id, pht, &dummy);

  cct_node_t *cct_child = (pht == gpu_placeholder_type_kernel_anon) ?
    host_op_node :
    hpcrun_cct_insert_ip_norm(host_op_node, ip, false);

  if (cct_child) {
    PRINT("cct_child %p\n", cct_child);
    sample->cct_node = cct_child;
  }
}


static void
gpu_sampling_info_process
(
 gpu_activity_t *sri
)
{
  uint64_t correlation_id = sri->details.pc_sampling_info.correlation_id;

  gpu_op_ccts_map_entry_value_t *entry = gpu_op_ccts_map_lookup(correlation_id);
  if (entry == NULL) {
    TMSG(GPU, "Cannot find CCTs for correlation id %"PRId64, correlation_id);
    return;
  }

  cct_node_t *func_ph =
    gpu_op_ccts_get(&entry->gpu_op_ccts, gpu_placeholder_type_trace);
  cct_node_t *kernel_ph =
    gpu_op_ccts_get(&entry->gpu_op_ccts, gpu_placeholder_type_kernel);

  cct_node_t *func_node = hpcrun_cct_children(func_ph); // only child
  cct_node_t *kernel_node;
  if (func_node == NULL) {
    // in case placeholder doesn't have a child
    kernel_node = kernel_ph;
  } else {
    // find the proper child of kernel_ph
    cct_addr_t *addr = hpcrun_cct_addr(func_node);
    kernel_node = hpcrun_cct_insert_ip_norm(kernel_ph, addr->ip_norm, true);

    sri->cct_node = kernel_node;
  }

  hpcrun_stats_acc_samples_add(sri->details.pc_sampling_info.totalSamples);
  hpcrun_stats_acc_samples_dropped_add(sri->details.pc_sampling_info.droppedSamples);
}


static void
gpu_memset_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.memset.correlation_id;

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, gpu_placeholder_type_memset, &dummy);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, activity->details.memset.start, host_op_node);

  gpu_context_stream_trace
    (activity->details.memset.device_id, activity->details.memset.context_id,
      activity->details.memset.stream_id, &entry_trace);

  activity->cct_node = host_op_node;

  //FIXME(keren): In OpenMP, an external_id may maps to multiple cct_nodes
  //gpu_host_correlation_map_delete(external_id);

  TMSG(GPU, "gpu_memset_process: id: 0x%lx  memKind: %d  bytes: %lu",
       correlation_id, activity->details.memset.memKind, activity->details.memset.bytes);

  PRINT("Memset correlation_id 0x%lx\n", correlation_id);
  PRINT("Memset kind %u\n", activity->details.memset.memKind);
  PRINT("Memset bytes %lu\n", activity->details.memset.bytes);
}


static void
gpu_kernel_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.kernel.correlation_id;

  ip_normalized_t kernel_ip;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  if (ip_normalized_eq(&kernel_ip, &ip_normalized_NULL)) {
    kernel_ip = activity->details.kernel.kernel_first_pc;
  }

  cct_node_t *kernel_node =
    hpcrun_cct_insert_ip_norm(ph, kernel_ip, true);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, activity->details.kernel.start, kernel_node);

  gpu_context_stream_trace
    (activity->details.kernel.device_id, activity->details.kernel.context_id,
      activity->details.kernel.stream_id, &entry_trace);

  activity->cct_node = kernel_node;

  PRINT("Kernel execution deviceId %u\n", activity->details.kernel.device_id);
  PRINT("Kernel execution correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_kernel_block_process
(
 gpu_activity_t* activity
)
{
  uint64_t correlation_id = activity->details.kernel_block.correlation_id;
  ip_normalized_t pc_ip = activity->details.kernel_block.pc;

  ip_normalized_t kernel_ip;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  // add the PC for the block as a child of the kernel placeholder
  activity->cct_node = hpcrun_cct_insert_ip_norm(ph, pc_ip, true);
}


static void
gpu_synchronization_process
(
 gpu_activity_t *activity
)
{
#if TRACK_SYNCHRONIZATION
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.synchronization.correlation_id;

  gpu_op_ccts_map_entry_value_t *entry = gpu_op_ccts_map_lookup(correlation_id);
  if (entry == NULL) {
    TMSG(GPU, "Cannot find CCTs for correlation id %"PRId64, correlation_id);
    return;
  }

  cct_node_t *host_op_node =
    gpu_op_ccts_get(&entry->gpu_op_ccts, gpu_placeholder_type_sync);
  activity->cct_node = host_op_node;

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, entry->cpu_submit_time, host_op_node);

  if (activity->kind == GPU_ACTIVITY_SYNCHRONIZATION) {
    uint32_t context_id = activity->details.synchronization.context_id;
    uint32_t stream_id = activity->details.synchronization.stream_id;
    uint32_t event_id = activity->details.synchronization.event_id;

    switch (activity->details.synchronization.syncKind) {
      case GPU_SYNC_STREAM:
      case GPU_SYNC_STREAM_EVENT_WAIT:
        // Insert a event for a specific stream
        PRINT("Add context %u stream %u sync\n", context_id, stream_id);
        gpu_context_stream_trace(IDTUPLE_INVALID, context_id, stream_id, &entry_trace);
        break;
      case GPU_SYNC_CONTEXT:
        // Insert events for all current active streams
        // TODO(Keren): What if the stream is created
        PRINT("Add context %u sync\n", context_id);
        gpu_trace_process_context(context_id, &entry_trace);
        break;
      case GPU_SYNC_EVENT:
        {
          // Find the corresponding stream that records the event
          gpu_event_id_map_entry_value_t *event_id_entry_value =
            gpu_event_id_map_lookup(event_id);
          if (event_id_entry_value != NULL) {
            context_id = event_id_entry_value->context_id;
            stream_id = event_id_entry_value->stream_id;
            PRINT("Add context %u stream %u event %u sync\n", context_id, stream_id, event_id);
            gpu_context_stream_trace(IDTUPLE_INVALID, context_id, stream_id, &entry_trace);
          }
          break;
        }
      default:
        // invalid
        PRINT("Synchronization correlation_id 0x%lx cannot be found\n",
              correlation_id);
    }
  }
  // TODO(Keren): handle event synchronization
  //FIXME(keren): In OpenMP, an external_id may maps to multiple cct_nodes
  //gpu_host_correlation_map_delete(external_id);

  PRINT("Synchronization correlation_id 0x%lx\n", correlation_id);
#endif
}


static void
gpu_cdpkernel_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.cdpkernel.correlation_id;

  gpu_op_ccts_map_entry_value_t *entry = gpu_op_ccts_map_lookup(correlation_id);
  if (entry == NULL) {
    TMSG(GPU, "Cannot find CCTs for correlation id %"PRId64, correlation_id);
    return;
  }

  cct_node_t *func_ph =
    gpu_op_ccts_get(&entry->gpu_op_ccts, gpu_placeholder_type_kernel);

  cct_node_t *func_node = hpcrun_leftmost_child(func_ph);
  if (func_node == NULL) {
    // in case placeholder doesn't have a child
    func_node = func_ph;
  }

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, entry->cpu_submit_time, func_node);

  gpu_context_stream_trace
    (activity->details.cdpkernel.device_id, activity->details.cdpkernel.context_id,
      activity->details.cdpkernel.stream_id, &entry_trace);


  PRINT("Cdp Kernel correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_event_process
(
 gpu_activity_t *activity
)
{
  uint32_t event_id = activity->details.event.event_id;
  uint32_t context_id = activity->details.event.context_id;
  uint32_t stream_id = activity->details.event.stream_id;
  gpu_event_id_map_insert(event_id, context_id, stream_id);

  PRINT("GPU event %u\n", event_id);
}

static gpu_placeholder_type_t
gpu_memory_placeholder
(
 gpu_activity_t *activity
)
{
  gpu_mem_op_t mem_op = activity->details.memory.mem_op;;
  switch(mem_op) {
  case GPU_MEM_OP_ALLOC: return gpu_placeholder_type_alloc;
  case GPU_MEM_OP_DELETE: return gpu_placeholder_type_delete;
  default:
    assert(false && "Invalid memory placeholder");
    hpcrun_terminate();
  }
  return gpu_placeholder_type_alloc;
}


static void
gpu_migration_process
(
 gpu_activity_t *activity
)
{
  uint64_t correlation_id = activity->details.migration.correlation_id;

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, gpu_placeholder_type_paging, &dummy);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu page migration: op_type=%s trigger=%s start=0x%lx end=0x%lx bytes: %lu",
       gpu_page_op_type_to_string(activity->details.migration.op_type),
       gpu_page_trigger_to_string(activity->details.migration.trigger),
       activity->details.migration.start_addr,  activity->details.migration.end_addr,
       activity->details.migration.bytes);
}


static gpu_placeholder_type_t
get_gpu_scratch_placeholder
(
  gpu_scratch_op_t op
)
{
  gpu_placeholder_type_t ph = gpu_placeholder_type_scratch_illegal;

  switch(op) {
    case GPU_SCRATCH_MEMORY_ALLOC:
      ph = gpu_placeholder_type_scratch_alloc;
      break;
    case GPU_SCRATCH_MEMORY_FREE:
      ph = gpu_placeholder_type_scratch_free;
      break;
    case GPU_SCRATCH_MEMORY_ASYNC_RECLAIM:
      ph = gpu_placeholder_type_scratch_async_reclaim;
      break;
    case GPU_SCRATCH_MEMORY_ILLEGAL:
      ph = gpu_placeholder_type_scratch_illegal;
      break;
  }
  return ph;
}

static void
gpu_scratch_process
(
 gpu_activity_t *activity
)
{
  uint64_t correlation_id = activity->details.scratch.correlation_id;
  gpu_placeholder_type_t ph =
    get_gpu_scratch_placeholder(activity->details.scratch.op_type);

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, ph, &dummy);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu scratch op: seconds %lf ",
      (activity->details.scratch.end - activity->details.scratch.start) / 1000000000);
}

static void
gpu_memory_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  uint64_t correlation_id = activity->details.memory.correlation_id;

  gpu_placeholder_type_t ph = gpu_memory_placeholder(activity);

  ip_normalized_t dummy;
  cct_node_t *host_op_node = get_placeholder_node(correlation_id, ph, &dummy);

  // Memory allocation does not always happen on the device
  // Do not send it to trace channels

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, activity->details.memory.start, host_op_node);

  gpu_context_stream_trace
    (activity->details.memory.device_id,
      activity->details.memory.context_id,
      activity->details.memory.stream_id,
      &entry_trace);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu_memory_process (alloc/free): id: 0x%lx  mem_op: %d  bytes: %lu",
       correlation_id, activity->details.memory.mem_op, activity->details.memory.bytes);

  PRINT("Memory correlation_id 0x%lx\n", correlation_id);
  PRINT("Memory kind %u\n", activity->details.memory.memKind);
  PRINT("Memory bytes %lu\n", activity->details.memory.bytes);
}


static void
gpu_instruction_process
(
 gpu_activity_t *activity
)
{
  uint64_t correlation_id = activity->details.instruction.correlation_id;
  ip_normalized_t pc = activity->details.instruction.pc;

  gpu_op_ccts_map_entry_value_t *entry = gpu_op_ccts_map_lookup(correlation_id);
  if (entry == NULL) {
    TMSG(GPU, "Cannot find CCTs for correlation id %"PRId64, correlation_id);
    return;
  }

  cct_node_t *func_ph =
    gpu_op_ccts_get(&entry->gpu_op_ccts, gpu_placeholder_type_kernel);

  cct_node_t *func_ins = hpcrun_cct_insert_ip_norm(func_ph, pc, true);
  activity->cct_node = func_ins;

  PRINT("Instruction correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_one_counter_process
(
 gpu_activity_t *activity
)
{
  uint64_t correlation_id = activity->details.counter.correlation_id;

  ip_normalized_t dummy;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &dummy);

  cct_node_t *kernel_node =
    hpcrun_cct_insert_ip_norm(ph, activity->details.counter.kernel_first_pc, true);

  activity->cct_node = kernel_node;
}


static void
gpu_counters_process
(
 gpu_activity_t *activity
)
{
  uint64_t correlation_id = activity->details.counters.correlation_id;

  gpu_op_ccts_map_entry_value_t *entry = gpu_op_ccts_map_lookup(correlation_id);
  if (entry == NULL) {
    TMSG(GPU, "Cannot find CCTs for correlation id %"PRId64, correlation_id);
    return;
  }

  gpu_placeholder_type_t ph = gpu_placeholder_type_kernel;
  cct_node_t *host_op_node = gpu_op_ccts_get(&entry->gpu_op_ccts, ph);
  if (host_op_node == NULL)
    hpcrun_terminate();

  cct_node_t *func_node = hpcrun_cct_children(host_op_node); // only child
  cct_node_t *kernel_node;
  if (func_node == NULL) {
    kernel_node = host_op_node;
  } else {
    cct_addr_t *addr = hpcrun_cct_addr(func_node);
    kernel_node = hpcrun_cct_insert_ip_norm(host_op_node, addr->ip_norm, true);
  }

  // Memory allocation does not always happen on the device
  // Do not send it to trace channels

  activity->cct_node = kernel_node;

  PRINT("Counter CorrelationId %u\n", correlation_id);

  // FIXME(Srdjan): activity->details.counters.* missing
  // PRINT("Counter cycles %lu\n", activity->details.counters.cycles);
  // PRINT("Counter l2 cache hit %lu\n", activity->details.counters.l2_cache_hit);
  // PRINT("Counter l2 cache miss %lu\n", activity->details.counters.l2_cache_miss);
}



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_activity_process
(
 gpu_activity_t *ga
)
{
  typedef void (*process_fn_t)(gpu_activity_t *);
  static const process_fn_t process_fns[] = {
    [GPU_ACTIVITY_PC_SAMPLING]        = gpu_sample_process,
    [GPU_ACTIVITY_PC_SAMPLING_INFO]   = gpu_sampling_info_process,
    [GPU_ACTIVITY_MEMCPY]             = gpu_memcpy_process,
    [GPU_ACTIVITY_MEMSET]             = gpu_memset_process,
    [GPU_ACTIVITY_KERNEL]             = gpu_kernel_process,
    [GPU_ACTIVITY_KERNEL_BLOCK]       = gpu_kernel_block_process,
    [GPU_ACTIVITY_SYNCHRONIZATION]    = gpu_synchronization_process,
    [GPU_ACTIVITY_MEMORY]             = gpu_memory_process,
    [GPU_ACTIVITY_LOCAL_ACCESS]       = gpu_instruction_process,
    [GPU_ACTIVITY_GLOBAL_ACCESS]      = gpu_instruction_process,
    [GPU_ACTIVITY_BRANCH]             = gpu_instruction_process,
    [GPU_ACTIVITY_CDP_KERNEL]         = gpu_cdpkernel_process,
    [GPU_ACTIVITY_EVENT]              = gpu_event_process,
    [GPU_ACTIVITY_COUNTERS]           = gpu_counters_process,
    [GPU_ACTIVITY_ONE_COUNTER]        = gpu_one_counter_process,
    [GPU_ACTIVITY_PAGE_MIGRATION]     = gpu_migration_process,
    [GPU_ACTIVITY_MEMCPY2]            = gpu_unknown_process,
    [GPU_ACTIVITY_SCRATCH]            = gpu_scratch_process
  };

  process_fn_t process_fn =
    ga->kind < sizeof(process_fns) / sizeof(process_fns[0]) && process_fns[ga->kind]
      ? process_fns[ga->kind]
      : gpu_unknown_process;

  process_fn(ga);
}
