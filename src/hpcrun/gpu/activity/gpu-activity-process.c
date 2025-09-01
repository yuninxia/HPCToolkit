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
  if (gpu_interval_is_invalid(&(gi->details.interval))) return



//******************************************************************************
// private operations
//******************************************************************************

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
 gpu_activity_t *activity,
 uint64_t cpu_submit_time,
 cct_node_t *call_path_leaf
)
{
  gpu_interval_t *interval = &activity->details.interval;
  gpu_trace_item_init(ti, cpu_submit_time, interval->start,
    interval->end, call_path_leaf);
}


static void
gpu_memcpy_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  gpu_memcpy_t *memcpy = &activity->details.memcpy;

  uint64_t correlation_id = memcpy->correlation_id;

  gpu_placeholder_type_t mct;
  switch (memcpy->copyKind) {
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
  trace_item_set(&entry_trace, activity, memcpy->start, host_op_node);

  gpu_context_stream_trace
    (memcpy->device_id, memcpy->context_id, memcpy->stream_id, &entry_trace);

  activity->cct_node = host_op_node;

  PRINT("Memcpy copy correlation_id 0x%lx\n", correlation_id);
  PRINT("Memcpy copy kind %u\n", memcpy->copyKind);
  PRINT("Memcpy copy bytes %lu\n", memcpy->bytes);
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
 gpu_activity_t *activity
)
{
  gpu_pc_sampling_t *pcs = &activity->details.pc_sampling;
  uint64_t correlation_id = pcs->correlation_id;

  ip_normalized_t ip = pcs->pc;

  gpu_placeholder_type_t pht =
    (ip.lm_id == 0) ? gpu_placeholder_type_kernel_anon : gpu_placeholder_type_kernel;

  ip_normalized_t dummy;
  cct_node_t *kernel_ph = get_placeholder_node(correlation_id, pht, &dummy);

  cct_node_t *pc = (pht == gpu_placeholder_type_kernel_anon) ?
    kernel_ph :
    hpcrun_cct_insert_ip_norm(kernel_ph, ip, false);

  if (pc) {
    PRINT("pc %p\n", pc);
    activity->cct_node = pc;
  }
}


static void
gpu_sampling_info_process
(
 gpu_activity_t *activity
)
{
  gpu_pc_sampling_info_t *pcs_info = &activity->details.pc_sampling_info;

  uint64_t correlation_id = pcs_info->correlation_id;

  ip_normalized_t kernel_ip;
  cct_node_t *kernel_ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  cct_node_t *kernel_node = ip_normalized_eq(&kernel_ip, &ip_normalized_NULL) ? kernel_ph :
    hpcrun_cct_insert_ip_norm(kernel_ph, kernel_ip, true);

  activity->cct_node = kernel_node;

  hpcrun_stats_acc_samples_add(pcs_info->totalSamples);
  hpcrun_stats_acc_samples_dropped_add(pcs_info->droppedSamples);
}


static void
gpu_memset_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  gpu_memset_t *memset = &activity->details.memset;

  uint64_t correlation_id = memset->correlation_id;

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, gpu_placeholder_type_memset, &dummy);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, memset->start, host_op_node);

  gpu_context_stream_trace
    (memset->device_id, memset->context_id, memset->stream_id, &entry_trace);

  activity->cct_node = host_op_node;

  //FIXME(keren): In OpenMP, an external_id may maps to multiple cct_nodes
  //gpu_host_correlation_map_delete(external_id);

  TMSG(GPU, "gpu_memset_process: id: 0x%lx  memKind: %d  bytes: %lu",
       correlation_id, memset->memKind, memset->bytes);

  PRINT("Memset correlation_id 0x%lx\n", correlation_id);
  PRINT("Memset kind %u\n", memset->memKind);
  PRINT("Memset bytes %lu\n", memset->bytes);
}


static void
gpu_kernel_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  gpu_kernel_t *kernel = &activity->details.kernel;

  uint64_t correlation_id = kernel->correlation_id;

  ip_normalized_t kernel_ip;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  if (ip_normalized_eq(&kernel_ip, &ip_normalized_NULL)) {
    kernel_ip = kernel->kernel_first_pc;
  }

  cct_node_t *kernel_node =
    hpcrun_cct_insert_ip_norm(ph, kernel_ip, true);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, kernel->start, kernel_node);

  gpu_context_stream_trace
    (kernel->device_id, kernel->context_id,
      kernel->stream_id, &entry_trace);

  activity->cct_node = kernel_node;

  PRINT("Kernel execution deviceId %u\n", kernel->device_id);
  PRINT("Kernel execution correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_kernel_block_process
(
 gpu_activity_t *activity
)
{
  gpu_kernel_block_t *kb = &activity->details.kernel_block;

  uint64_t correlation_id = kb->correlation_id;
  ip_normalized_t pc_ip = kb->pc;

  ip_normalized_t kernel_ip;
  cct_node_t *kernel_ph =
    get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  // add the PC for the block as a child of the kernel placeholder
  activity->cct_node = hpcrun_cct_insert_ip_norm(kernel_ph, pc_ip, true);
}


static void
gpu_synchronization_process
(
 gpu_activity_t *activity
)
{
#if TRACK_SYNCHRONIZATION
  IGNORE_IF_INVALID_INTERVAL(activity);

  gpu_synchronization_t *sync = &activity->details.synchronization;

  uint64_t correlation_id = sync->correlation_id;

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, gpu_placeholder_type_sync, &dummy);

  activity->cct_node = host_op_node;

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, entry->cpu_submit_time, host_op_node);

  if (activity->kind == GPU_ACTIVITY_SYNCHRONIZATION) {
    uint32_t context_id = sync->context_id;
    uint32_t stream_id = sync->stream_id;
    uint32_t event_id = sync->event_id;

    switch (sync->syncKind) {
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

  gpu_cdpkernel_t *cdpk = &activity->details.cdpkernel;

  uint64_t correlation_id = cdpk->correlation_id;

  ip_normalized_t kernel_ip;
  cct_node_t *kernel_ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &kernel_ip);

  cct_node_t *kernel_node = ip_normalized_eq(&kernel_ip, &ip_normalized_NULL) ? kernel_ph :
    hpcrun_cct_insert_ip_norm(kernel_ph, kernel_ip, true);

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, cdpk->start, kernel_node);

  gpu_context_stream_trace
    (cdpk->device_id, cdpk->context_id, cdpk->stream_id, &entry_trace);

  activity->cct_node = kernel_node;

  PRINT("Cdp Kernel correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_event_process
(
 gpu_activity_t *activity
)
{
  gpu_event_t *event = &activity->details.event;

  gpu_event_id_map_insert(event->event_id, event->context_id,
    event->stream_id);

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
  gpu_page_migration_t *migration = &activity->details.migration;

  uint64_t correlation_id = migration->correlation_id;

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, gpu_placeholder_type_paging, &dummy);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu page migration: op_type=%s trigger=%s "
    "start=0x%lx end=0x%lx bytes: %lu",
    gpu_page_op_type_to_string(migration->op_type),
    gpu_page_trigger_to_string(migration->trigger), migration->start_addr,
    migration->end_addr, migration->bytes);
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
  gpu_scratch_t *scratch = &activity->details.scratch;
  uint64_t correlation_id = scratch->correlation_id;

  gpu_placeholder_type_t ph =
    get_gpu_scratch_placeholder(scratch->op_type);

  ip_normalized_t dummy;
  cct_node_t *host_op_node =
    get_placeholder_node(correlation_id, ph, &dummy);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu scratch op: seconds %lf ",
      (scratch->end - scratch->start) / 1000000000);
}

static void
gpu_memory_process
(
 gpu_activity_t *activity
)
{
  IGNORE_IF_INVALID_INTERVAL(activity);

  gpu_memory_t *memory = &activity->details.memory;

  uint64_t correlation_id = memory->correlation_id;

  gpu_placeholder_type_t ph = gpu_memory_placeholder(activity);

  ip_normalized_t dummy;
  cct_node_t *host_op_node = get_placeholder_node(correlation_id, ph, &dummy);

  // Memory allocation does not always happen on the device
  // Do not send it to trace channels

  gpu_trace_item_t entry_trace;
  trace_item_set(&entry_trace, activity, memory->start, host_op_node);

  gpu_context_stream_trace
    (memory->device_id, memory->context_id, memory->stream_id, &entry_trace);

  activity->cct_node = host_op_node;

  TMSG(GPU, "gpu_memory_process (alloc/free): id: 0x%lx  mem_op: %d  bytes: %lu",
       correlation_id, memory->mem_op, memory->bytes);

  PRINT("Memory correlation_id 0x%lx\n", correlation_id);
  PRINT("Memory kind %u\n", memory->memKind);
  PRINT("Memory bytes %lu\n", memory->bytes);
}


static void
gpu_instruction_process
(
 gpu_activity_t *activity
)
{
  gpu_instruction_t *inst = &activity->details.instruction;

  uint64_t correlation_id = inst->correlation_id;
  ip_normalized_t ip = inst->pc;

  gpu_placeholder_type_t pht =
    (ip.lm_id == 0) ? gpu_placeholder_type_kernel_anon : gpu_placeholder_type_kernel;

  ip_normalized_t dummy;
  cct_node_t *host_op_node = get_placeholder_node(correlation_id, pht, &dummy);

  cct_node_t *inst_node =
    hpcrun_cct_insert_ip_norm(host_op_node, ip, false);

  if (inst_node != host_op_node) {
    PRINT("inst_node %p\n", inst_node);
    activity->cct_node = inst_node;
  }

  PRINT("Instruction correlation_id 0x%lx\n", correlation_id);
}


static void
gpu_one_counter_process
(
 gpu_activity_t *activity
)
{
  gpu_one_counter_t *counter = &activity->details.counter;

  uint64_t correlation_id = counter->correlation_id;

  ip_normalized_t dummy;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &dummy);

  cct_node_t *kernel_node =
    hpcrun_cct_insert_ip_norm(ph, counter->kernel_first_pc, true);

  activity->cct_node = kernel_node;
}


static void
gpu_counters_process
(
 gpu_activity_t *activity
)
{
  gpu_counter_t *counters = &activity->details.counters;
  uint64_t correlation_id = counters->correlation_id;

  ip_normalized_t dummy;
  cct_node_t *ph = get_placeholder_node(correlation_id, gpu_placeholder_type_kernel, &dummy);

  cct_node_t *kernel_node =
    hpcrun_cct_insert_ip_norm(ph, counters->kernel_first_pc, true);

  activity->cct_node = kernel_node;

  PRINT("Counter CorrelationId %u\n", correlation_id);

  // FIXME(Srdjan): activity->details.counters.* missing
  // PRINT("Counter cycles %lu\n", counters->cycles);
  // PRINT("Counter l2 cache hit %lu\n", counters->l2_cache_hit);
  // PRINT("Counter l2 cache miss %lu\n", counters->l2_cache_miss);
}



//******************************************************************************
// interface operations
//******************************************************************************

void
gpu_activity_process
(
 gpu_activity_t *activity
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
    (activity->kind < sizeof(process_fns) / sizeof(process_fns[0]) &&
     process_fns[activity->kind]) ?
    process_fns[activity->kind] : gpu_unknown_process;

  process_fn(activity);
}
