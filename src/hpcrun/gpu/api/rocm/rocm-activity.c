// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#include <string.h>
#include <threads.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"
#include "../../../gpu/activity/gpu-activity-channel.h"
#include "../../../gpu/activity/gpu-activity-process.h"
#include "../../../gpu/activity/gpu-op-placeholders.h"
#include "../../../utilities/hpcrun-nanotime.h"

#include "rocm.h"
#include "rocm-activity.h"
#include "rocm-api.h"
#include "rocm-codeobject-map.h"
#include "rocm-extid.h"
#include "rocm-regex.h"
#include "rocm-symbol-map.h"
#include "rocm-utils.h"



//******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// macros
//******************************************************************************

#define TRACK_SYNCHRONIZATION 0

// set the constant below to 1 to test handling of for pc samples with
// unknown (0) load module. they will be mapped to <gpu kernel anonymous>
#define TEST_ANONYMOUS_KERNEL 0

// set the constant below to 1 to test handling of for pc samples that
// have no correlation id, e.g. cid == 0
#define TEST_NO_LOCATION 0

// set the constant below to 1 to test scratch record processing
// with synthetic measurements
#define TEST_SCRATCH 0

#define KIND_CASE(amd_kind, hpctoolkit_kind) \
  case amd_kind: kind = hpctoolkit_kind; break;

#define CONVERT_HIP_RUNTIME(ga, activity, dtype, record)  \
  (ga)->kind = (activity);  \
  (ga)->details.dtype.start = activity_time((record)->start_timestamp);  \
  (ga)->details.dtype.end = activity_time((record)->end_timestamp);  \
  (ga)->details.dtype.correlation_id = (record)->correlation_id.external.value



//******************************************************************************
// private data
//******************************************************************************

static uint64_t clock_offset_ns;

//******************************************************************************
// private data
//******************************************************************************

static uint64_t clock_offset_ns;


//******************************************************************************
// private operations
//******************************************************************************

static void
clock_offset_ns_init
(
  void
)
{
  clock_offset_ns = hpcrun_nanotime_real_boot_offset();
}


uint64_t
activity_time
(
  rocprofiler_timestamp_t t
)
{
  static once_flag once = ONCE_FLAG_INIT;
  call_once(&once, clock_offset_ns_init);

  return t + clock_offset_ns;
}


uint64_t
extract_external_correlation_id
(
  rocprofiler_correlation_id_t *id
)
{
  return id->external.value;
}



uint64_t
extract_external_async_correlation_id
(
  rocprofiler_async_correlation_id_t *id
)
{
  return id->external.value;
}


static uint64_t
extract_device_id
(
  rocprofiler_agent_id_t *id
)
{
  return id->handle;
}


static uint64_t
extract_queue_id
(
  rocprofiler_queue_id_t *id
)
{
  return id->handle;
}


uint64_t
dim3_size
(
  rocprofiler_dim3_t *dim3
)
{
  return dim3->x * dim3->y * dim3->z;
}


static uint64_t
convert_kernel_launch
(
 gpu_activity_t *ga,
 rocprofiler_buffer_tracing_kernel_dispatch_record_t* activity
)
{
  ga->kind = GPU_ACTIVITY_KERNEL;
  gpu_interval_set(&ga->details.interval, activity_time(activity->start_timestamp),
    activity_time(activity->end_timestamp));

  ga->details.kernel.correlation_id = extract_external_async_correlation_id(&activity->correlation_id);

  ga->details.kernel.device_id = extract_device_id(&activity->dispatch_info.agent_id);
  ga->details.kernel.context_id = extract_device_id(&activity->dispatch_info.agent_id);
  ga->details.kernel.stream_id = extract_queue_id(&activity->dispatch_info.queue_id);

  rocm_symbol_info_t *info = rocm_symbol_map_find(activity->dispatch_info.kernel_id);

  ga->details.kernel.kernel_first_pc = (info) ? info->kernel_ip : ip_normalized_NULL;
  ga->details.kernel.scalarRegisters = (info) ? info->kernel_sgpr_count : 0;
  ga->details.kernel.vectorRegisters = (info) ? info->kernel_vgpr_count : 0;
  ga->details.kernel.blockSharedMemory = (info) ? info->workgroup_LDS_bytes : 0;
  ga->details.kernel.localMemoryTotal = (info) ? info->kernel_scratch_bytes : 0;
  ga->details.kernel.blockThreads = dim3_size(&(activity->dispatch_info.workgroup_size));
  ga->details.kernel.blocks = dim3_size(&(activity->dispatch_info.grid_size));

  return ga->details.kernel.correlation_id;
}


static gpu_memcpy_type_t
convert_memcpy_kind
(
  rocprofiler_memory_copy_operation_t operation
)
{
  gpu_memcpy_type_t kind;

#define MEMCPY_KIND(MACRO) \
  MACRO(ROCPROFILER_MEMORY_COPY_HOST_TO_HOST,     GPU_MEMCPY_H2H)  \
  MACRO(ROCPROFILER_MEMORY_COPY_HOST_TO_DEVICE,   GPU_MEMCPY_H2D)  \
  MACRO(ROCPROFILER_MEMORY_COPY_DEVICE_TO_HOST,   GPU_MEMCPY_D2H)  \
  MACRO(ROCPROFILER_MEMORY_COPY_DEVICE_TO_DEVICE, GPU_MEMCPY_D2D)

  switch(operation) {
  MEMCPY_KIND(KIND_CASE)
  default:
    kind = GPU_MEMCPY_UNK;
    break;
  }

  return kind;
}


static gpu_scratch_op_t
convert_scratch_op
(
  rocprofiler_scratch_memory_operation_t operation
)
{
  gpu_scratch_op_t kind;

#define SCRATCH_KIND(MACRO) \
  MACRO(ROCPROFILER_SCRATCH_MEMORY_ALLOC, GPU_SCRATCH_MEMORY_ALLOC)                 \
  MACRO(ROCPROFILER_SCRATCH_MEMORY_FREE, GPU_SCRATCH_MEMORY_FREE)                   \
  MACRO(ROCPROFILER_SCRATCH_MEMORY_ASYNC_RECLAIM, GPU_SCRATCH_MEMORY_ASYNC_RECLAIM)

  switch(operation) {
  SCRATCH_KIND(KIND_CASE)
  default: kind = GPU_SCRATCH_MEMORY_ILLEGAL; break;
  }

  return kind;
}


static uint64_t
convert_memcpy
(
 gpu_activity_t *ga,
 rocprofiler_buffer_tracing_memory_copy_record_t *activity,
 gpu_memcpy_type_t kind
)
{
  ga->kind = GPU_ACTIVITY_MEMCPY;
  gpu_interval_set(&ga->details.interval, activity_time(activity->start_timestamp),
    activity_time(activity->end_timestamp));
  ga->details.memcpy.correlation_id = extract_external_async_correlation_id(&activity->correlation_id);

  ga->details.memcpy.copyKind = kind;
  ga->details.memcpy.bytes = activity->bytes;

  ga->details.memcpy.device_id =  extract_device_id(&activity->dst_agent_id);
  ga->details.memcpy.context_id = extract_device_id(&activity->dst_agent_id);
  ga->details.memcpy.stream_id = 0; // only device information is available; no queue

  return ga->details.memcpy.correlation_id;
}


static uint64_t
convert_scratch_memory
(
  gpu_activity_t *ga,
  rocprofiler_buffer_tracing_scratch_memory_record_t *activity
)
{
  // see openmp_target sample for further details
  ga->kind = GPU_ACTIVITY_SCRATCH;
  gpu_interval_set(&ga->details.interval, activity_time(activity->start_timestamp),
    activity_time(activity->end_timestamp));
  ga->details.scratch.correlation_id = extract_external_correlation_id(&activity->correlation_id);
  ga->details.scratch.op_type = convert_scratch_op(activity->operation);
  return ga->details.scratch.correlation_id;
}


#ifdef ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION
static gpu_mem_op_t
convert_memory_alloc_kind
(
  rocprofiler_memory_allocation_operation_t operation
)
{
  gpu_mem_op_t kind;

#define ALLOC_KIND(MACRO) \
  MACRO(ROCPROFILER_MEMORY_ALLOCATION_ALLOCATE,      GPU_MEM_OP_ALLOC)   \
  MACRO(ROCPROFILER_MEMORY_ALLOCATION_VMEM_ALLOCATE, GPU_MEM_OP_ALLOC)   \
  MACRO(ROCPROFILER_MEMORY_ALLOCATION_FREE,          GPU_MEM_OP_DELETE)  \
  MACRO(ROCPROFILER_MEMORY_ALLOCATION_VMEM_FREE,     GPU_MEM_OP_DELETE)

  switch(operation) {
  ALLOC_KIND(KIND_CASE)
  default:
    kind = GPU_MEM_OP_UNKNOWN;
    break;
  }

  return kind;
}


static uint64_t
convert_memory_alloc
(
 gpu_activity_t *ga,
 rocprofiler_buffer_tracing_memory_allocation_record_t *activity
)
{
  ga->kind = GPU_ACTIVITY_MEMORY;

  gpu_interval_set(&ga->details.interval, activity_time(activity->start_timestamp),
    activity_time(activity->end_timestamp));

  ga->details.memory.correlation_id = extract_external_correlation_id(&activity->correlation_id);
  ga->details.memory.device_id = extract_device_id(&activity->agent_id);
  ga->details.memory.mem_op = convert_memory_alloc_kind(activity->operation);
  ga->details.memory.memKind = GPU_MEM_UNKNOWN;
  ga->details.memory.bytes = activity->allocation_size;

  return ga->details.memory.correlation_id;
}
#endif


#ifdef ROCPROFILER_PAGE_MIGRATION_AVAILABLE
static gpu_page_op_trigger_t
convert_page_migration_trigger
(
  rocprofiler_page_migration_trigger_t trigger
)
{
  gpu_page_op_trigger_t kind;

#define MIGRATION_TRIGGER_TO_TYPE(MACRO) \
  MACRO(ROCPROFILER_PAGE_MIGRATION_TRIGGER_NONE, GPU_PAGE_MIGRATE_TRIGGER_UNKNOWN)      \
  MACRO(ROCPROFILER_PAGE_MIGRATION_TRIGGER_PREFETCH, GPU_PAGE_MIGRATE_PREFETCH)         \
  MACRO(ROCPROFILER_PAGE_MIGRATION_TRIGGER_PAGEFAULT_GPU, GPU_PAGE_MIGRATE_FAULT_GPU)   \
  MACRO(ROCPROFILER_PAGE_MIGRATION_TRIGGER_PAGEFAULT_CPU, GPU_PAGE_MIGRATE_FAULT_CPU)   \
  MACRO(ROCPROFILER_PAGE_MIGRATION_TRIGGER_TTM_EVICTION, GPU_PAGE_MIGRATE_TTM_EVICTION)

  switch (trigger) {
    MIGRATION_TRIGGER_TO_TYPE(KIND_CASE)
    default: kind = GPU_PAGE_TRIGGER_NOTFOUND; break;
  }
  return kind;
}


static gpu_page_op_trigger_t
convert_page_queue_suspend_trigger
(
  rocprofiler_page_migration_queue_suspend_trigger_t trigger
)
{
  gpu_page_op_trigger_t kind;

#define SUSPEND_TRIGGER_TO_TYPE(MACRO) \
  MACRO(ROCPROFILER_PAGE_MIGRATION_QUEUE_SUSPEND_TRIGGER_NONE, GPU_PAGE_QUEUE_SUSPEND_TRIGGER_UNKNOWN) \
  MACRO(ROCPROFILER_PAGE_MIGRATION_QUEUE_SUSPEND_TRIGGER_SVM, GPU_PAGE_QUEUE_SUSPEND_SVM)              \
  MACRO(ROCPROFILER_PAGE_MIGRATION_QUEUE_SUSPEND_TRIGGER_USERPTR, GPU_PAGE_QUEUE_SUSPEND_USERPTR)      \
  MACRO(ROCPROFILER_PAGE_MIGRATION_QUEUE_SUSPEND_TRIGGER_TTM, GPU_PAGE_QUEUE_SUSPEND_TTM)              \
  MACRO(ROCPROFILER_PAGE_MIGRATION_QUEUE_SUSPEND_TRIGGER_SUSPEND, GPU_PAGE_QUEUE_SUSPEND_SUSPEND)

  switch (trigger) {
    SUSPEND_TRIGGER_TO_TYPE(KIND_CASE)
    default: kind = GPU_PAGE_TRIGGER_NOTFOUND; break;
  }

  return kind;
}


static gpu_page_op_trigger_t
convert_page_unmap_trigger
(
  rocprofiler_page_migration_unmap_from_gpu_trigger_t trigger
)
{
  gpu_page_op_trigger_t kind;

#define UNMAP_TRIGGER_TO_TYPE(MACRO) \
  MACRO(ROCPROFILER_PAGE_MIGRATION_UNMAP_FROM_GPU_TRIGGER_NONE, GPU_PAGE_UNMAP_TRIGGER_UNKNOWN)                           \
  MACRO(ROCPROFILER_PAGE_MIGRATION_UNMAP_FROM_GPU_TRIGGER_MMU_NOTIFY, GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY)                 \
  MACRO(ROCPROFILER_PAGE_MIGRATION_UNMAP_FROM_GPU_TRIGGER_MMU_NOTIFY_MIGRATE, GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY_MIGRATE) \
  MACRO(ROCPROFILER_PAGE_MIGRATION_UNMAP_FROM_GPU_TRIGGER_UNMAP_FROM_CPU, GPU_PAGE_UNMAP_FROM_CPU)

  switch (trigger) {
    UNMAP_TRIGGER_TO_TYPE(KIND_CASE)
    default: kind = GPU_PAGE_TRIGGER_NOTFOUND; break;
  }
  return kind;
}


static void
migration_decode
(
  gpu_activity_t *ga,
  rocprofiler_buffer_tracing_page_migration_record_t *activity
)
{
  gpu_page_op_trigger_t trigger = GPU_PAGE_TRIGGER_NONE;
  uint64_t start_addr = 0;
  uint64_t end_addr = 0;
  uint64_t bytes = 0;
  gpu_page_op_type_t op_type;

  switch(activity->operation) {
    case ROCPROFILER_PAGE_MIGRATION_PAGE_MIGRATE_START:
      op_type = GPU_PAGE_MIGRATE_START;
      start_addr = activity->args.page_migrate_start.start_addr;
      end_addr = activity->args.page_migrate_start.end_addr;
      bytes = activity->args.page_migrate_start.end_addr -
        activity->args.page_migrate_start.start_addr;
      trigger = convert_page_migration_trigger(activity->args.page_migrate_start.trigger);
      break;

    case ROCPROFILER_PAGE_MIGRATION_PAGE_MIGRATE_END:
      op_type = GPU_PAGE_MIGRATE_END;
      start_addr = activity->args.page_migrate_end.start_addr;
      end_addr = activity->args.page_migrate_end.end_addr;
      bytes = activity->args.page_migrate_end.end_addr -
        activity->args.page_migrate_end.start_addr;
      trigger = convert_page_migration_trigger(activity->args.page_migrate_end.trigger);
      break;

    case ROCPROFILER_PAGE_MIGRATION_PAGE_FAULT_START:
      op_type = GPU_PAGE_FAULT_START;
      break;

    case ROCPROFILER_PAGE_MIGRATION_PAGE_FAULT_END:
      op_type = GPU_PAGE_FAULT_END;
      break;

    case ROCPROFILER_PAGE_MIGRATION_QUEUE_EVICTION:
      op_type = GPU_PAGE_QUEUE_EVICTION;
      trigger = convert_page_queue_suspend_trigger(activity->args.queue_eviction.trigger);
      break;

    case ROCPROFILER_PAGE_MIGRATION_QUEUE_RESTORE:
      op_type = GPU_PAGE_QUEUE_RESTORE;
      break;

    case ROCPROFILER_PAGE_MIGRATION_UNMAP_FROM_GPU:
      op_type = GPU_PAGE_UNMAP;
      start_addr = activity->args.unmap_from_gpu.start_addr;
      end_addr = activity->args.unmap_from_gpu.end_addr;
      bytes = activity->args.unmap_from_gpu.end_addr -
        activity->args.unmap_from_gpu.start_addr;
      trigger = convert_page_unmap_trigger(activity->args.unmap_from_gpu.trigger);
      break;

    case ROCPROFILER_PAGE_MIGRATION_DROPPED_EVENT:
      op_type = GPU_PAGE_DROPPED_EVENT;
      ga->details.migration.dropped_events_count =
        activity->args.dropped_event.dropped_events_count;
      break;

    case ROCPROFILER_PAGE_MIGRATION_NONE:
      op_type = GPU_PAGE_NONE;
      break;

    default:
      // missing case or corrupt data at runtime
      assert(0);
  }

  ga->details.migration.op_type = op_type;
  ga->details.migration.start_addr = start_addr;
  ga->details.migration.end_addr = end_addr;
  ga->details.migration.trigger = trigger;
  ga->details.migration.bytes = bytes;
}


static uint64_t
convert_page_migration
(
 gpu_activity_t *ga,
 rocprofiler_buffer_tracing_page_migration_record_t *activity
)
{
  ga->kind = GPU_ACTIVITY_PAGE_MIGRATION;

  ga->details.migration.timestamp = activity_time(activity->timestamp);

  migration_decode(ga, activity);

  // correlation id that inserts into <gpu runtime>
  ga->details.migration.correlation_id = GPU_RUNTIME_PH_CID;

  return ga->details.migration.correlation_id;
}
#endif


#if 0
static void
convert_memset
(
 gpu_activity_t *ga,
 roctracer_record_t *activity
)
{
  ga->kind = GPU_ACTIVITY_MEMSET;
  gpu_interval_set(&ga->details.interval, activity->begin_ns, activity->end_ns);
  ga->details.memset.correlation_id = activity->correlation_id;
  ga->details.memset.context_id = activity->device_id;
  ga->details.memset.stream_id = activity->queue_id;
}


#if TRACK_SYNCHRONIZATION
static void
convert_sync
(
 gpu_activity_t *ga,
 roctracer_record_t *activity,
 gpu_sync_type_t syncKind
)
{
  ga->kind = GPU_ACTIVITY_SYNCHRONIZATION;
  gpu_interval_set(&ga->details.interval, activity->begin_ns, activity->end_ns);
  ga->details.synchronization.syncKind = syncKind;
  ga->details.synchronization.correlation_id = activity->correlation_id;
  ga->details.synchronization.context_id = activity->device_id;
  ga->details.synchronization.stream_id = activity->queue_id;
}
#endif
#endif

#ifdef ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC
static gpu_inst_stall_t
convert_stall_type
(
 rocprofiler_pc_sampling_record_hw_t *pcs
)
{
  return GPU_INST_STALL_NONE;
}


static uint64_t
convert_pc_sampling_hw
(
 gpu_activity_t *ga,
 rocprofiler_pc_sampling_record_hw_t *pcs
)
{
  PRINT("PC sampling: sample(" PC_FORMAT ", ts=%ld, exec_mask=0x%16lx, "
      "wgid=(x=%d, y=%d, z=%d), wave_id=%u, icid=0x%lx ecid=0x%lx)\n",
      PC_VALUE(pcs->pc), pcs->timestamp, pcs->exec_mask, pcs->workgroup_id.x,
      pcs->workgroup_id.y, pcs->workgroup_id.z, pcs->wave_in_group,
      pcs->correlation_id.internal, pcs->correlation_id.external.value);

  ga->kind = GPU_ACTIVITY_PC_SAMPLING;

  ip_normalized_t pc = rocm_codeobject_map_normalize(pcs->pc);

  uint64_t cid = pcs->correlation_id.external.value;

#if TEST_ANONYMOUS_KERNEL
  pc = gpu_op_placeholder_ip(gpu_placeholder_type_kernel_anon);
#endif

#if TEST_NO_LOCATION
  cid = 0;
#endif

  if (cid == 0) {
    if (pc.lm_id == 0) {
      // <gpu kernel anonymous> goes under <gpu runtime>
      cid = GPU_RUNTIME_PH_CID;
    } else {
      // known kernels go under <partial callpaths>
      cid = PARTIAL_UNWIND_PH_CID;
    }
  }

  PRINT("PC sample IP = [%d, 0x%lx]\n", pc.lm_id, pc.lm_ip);

  // if a sample has no correlation id, then we assign it a
  // correlation id saying it is a sample in the gpu runtime.
  if (cid == 0) {
    cid = GPU_RUNTIME_PH_CID;
  }

  ga->details.instruction.correlation_id = cid;

  ga->details.instruction.pc = pc;

  ga->details.pc_sampling.stallReason = convert_stall_type(pcs);
  ga->details.pc_sampling.samples = 1;
  ga->details.pc_sampling.latencySamples = 0;

  PRINT("PC sample GA: pc [0x%d, 0x%lx], corr 0x%lx, "
        "samples %u, latencySamples %u, stallReason %u\n",
        ga->details.instruction.pc.lm_id, ga->details.instruction.pc.lm_ip,
        ga->details.instruction.correlation_id,
        ga->details.pc_sampling.samples,
        ga->details.pc_sampling.latencySamples,
        ga->details.pc_sampling.stallReason);

  return cid;
}
#endif

static uint64_t
convert_pc_sampling_sw
(
 gpu_activity_t *ga,
 rocprofiler_pc_sampling_record_sw_t *pcs
)
{
  PRINT("PC sampling: sample(" PC_FORMAT ", ts=%ld, exec_mask=0x%16lx, "
      "wgid=(x=%d, y=%d, z=%d), wave_id=%u, icid=0x%lx ecid=0x%lx)\n",
      PC_VALUE(pcs->pc), pcs->timestamp, pcs->exec_mask, pcs->workgroup_id.x,
      pcs->workgroup_id.y, pcs->workgroup_id.z, pcs->wave_in_group,
      pcs->correlation_id.internal, pcs->correlation_id.external.value);

  ga->kind = GPU_ACTIVITY_PC_SAMPLING;

  ip_normalized_t pc = rocm_codeobject_map_normalize(pcs->pc);

  uint64_t cid = pcs->correlation_id.external.value;

#if TEST_ANONYMOUS_KERNEL
  pc = gpu_op_placeholder_ip(gpu_placeholder_type_kernel_anon);
#endif

#if TEST_NO_LOCATION
  cid = 0;
#endif

  if (cid == 0) {
    if (pc.lm_id == 0) {
      // <gpu kernel anonymous> goes under <gpu runtime>
      cid = GPU_RUNTIME_PH_CID;
    } else {
      // known kernels go under <partial callpaths>
      cid = PARTIAL_UNWIND_PH_CID;
    }
  }

  PRINT("PC sample IP = [%d, 0x%lx]\n", pc.lm_id, pc.lm_ip);

  // if a sample has no correlation id, then we assign it a
  // correlation id saying it is a sample in the gpu runtime.
  if (cid == 0) {
    cid = GPU_RUNTIME_PH_CID;
  }

  ga->details.instruction.correlation_id = cid;

  ga->details.instruction.pc = pc;

  ga->details.pc_sampling.stallReason = GPU_INST_STALL_NONE;
  ga->details.pc_sampling.samples = 1;
  ga->details.pc_sampling.latencySamples = 0;

  PRINT("PC sample GA: pc [0x%d, 0x%lx], corr 0x%lx, "
        "samples %u, latencySamples %u, stallReason %u\n",
        ga->details.instruction.pc.lm_id, ga->details.instruction.pc.lm_ip,
        ga->details.instruction.correlation_id,
        ga->details.pc_sampling.samples,
        ga->details.pc_sampling.latencySamples,
        ga->details.pc_sampling.stallReason);

  return cid;
}


static void
convert_unknown
(
 gpu_activity_t *ga
)
{
  ga->kind = GPU_ACTIVITY_UNKNOWN;
}


static uint64_t
rocm_activity_translate_buffer_tracing
(
  gpu_activity_t *ga,
  rocprofiler_record_header_t *header
)
{
  uint64_t cid = 0;

  switch(header->kind) {
    case ROCPROFILER_BUFFER_TRACING_KERNEL_DISPATCH: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_kernel_dispatch_record_t*,
            record, header->payload);
        cid = convert_kernel_launch(ga, record);
      }
      break;

#ifdef ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION
    case ROCPROFILER_BUFFER_TRACING_MEMORY_ALLOCATION: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_memory_allocation_record_t*,
            record, header->payload);
        cid = convert_memory_alloc(ga, record);
      }
      break;
#endif

    case ROCPROFILER_BUFFER_TRACING_MEMORY_COPY: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_memory_copy_record_t*,
            record, header->payload);
        cid = convert_memcpy(ga, record, convert_memcpy_kind(record->operation));
      }
      break;

    case ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_scratch_memory_record_t *,
            record, header->payload);
        cid = convert_scratch_memory(ga, record);
      }
      break;

#ifdef ROCPROFILER_PAGE_MIGRATION_AVAILABLE
    // page migration support only defined for ROCm 6.4+
    case ROCPROFILER_BUFFER_TRACING_PAGE_MIGRATION: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_page_migration_record_t*,
            record, header->payload);
        cid = convert_page_migration(ga, record);
      }
      break;
#endif

    case ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API: {
        DECL_INIT_CAST
          (rocprofiler_buffer_tracing_hip_api_record_t *,
          record, header->payload);

        uint32_t op = record->operation;

        switch(rocm_hip_runtime_activity_vec[op].major) {

        case GPU_ACTIVITY_MEMORY:
          TMSG(ROCM, "activity_translate_buffer_tracing: GPU_ACTIVITY_MEMORY: %d", op);
          CONVERT_HIP_RUNTIME(ga, GPU_ACTIVITY_MEMORY, memory, record);
          ga->details.memory.memKind = GPU_MEM_UNKNOWN;
          ga->details.memory.mem_op = rocm_hip_runtime_activity_vec[op].minor;
          // fixme: fake number to show up in the viewer
          ga->details.memory.bytes = 1;
          cid = ga->details.memory.correlation_id;
          break;

        case GPU_ACTIVITY_MEMSET:
          TMSG(ROCM, "activity_translate_buffer_tracing: GPU_ACTIVITY_MEMSET: %d", op);
          CONVERT_HIP_RUNTIME(ga, GPU_ACTIVITY_MEMSET, memset, record);
          ga->details.memset.memKind = GPU_MEM_UNKNOWN;
          // fixme: fake number to show up in the viewer
          ga->details.memset.bytes = 1;
          cid = ga->details.memory.correlation_id;
          break;

  #if 0
        case GPU_ACTIVITY_SYNCHRONIZATION:
          TMSG(ROCM, "activity_translate_buffer_tracing: GPU_ACTIVITY_SYNC: %d", op);
          CONVERT_HIP_RUNTIME(ga, GPU_ACTIVITY_SYNCHRONIZATION, synchronization, record);
          ga->details.synchronization.syncKind = rocm_hip_runtime_activity_vec[op].minor;
          cid = ga->details.synchronization.correlation_id;
          break;

        case GPU_ACTIVITY_MEMCPY:
          TMSG(ROCM, "activity_translate_buffer_tracing: GPU_ACTIVITY_MEMCPY: %d", op);
          CONVERT_HIP_RUNTIME(ga, GPU_ACTIVITY_MEMCPY, memcpy, record);
          ga->details.memcpy.copyKind = rocm_hip_runtime_activity_vec[op].minor;
          // fixme: fake number to show up in the viewer
          ga->details.memcpy.bytes = 1;
          cid = ga->details.memcpy.correlation_id;
          break;
  #endif

        default:
          TMSG(ROCM, "activity_translate_buffer_tracing: UNKNOWN: %d", op);
          convert_unknown(ga);
          break;
        }
      }
      break;

    case ROCPROFILER_BUFFER_TRACING_HSA_CORE_API:
    case ROCPROFILER_BUFFER_TRACING_HSA_AMD_EXT_API:
      // see rocprofiler_hsa_amd_ext_api_id_t
    case ROCPROFILER_BUFFER_TRACING_HSA_IMAGE_EXT_API:
      // see rocprofiler_hsa_image_ext_api_id_t
    case ROCPROFILER_BUFFER_TRACING_HSA_FINALIZE_EXT_API:
      // see rocprofiler_hsa_finalize_ext_api_id_t
    case ROCPROFILER_BUFFER_TRACING_HIP_COMPILER_API:
      // see rocprofiler_hip_compiler_api_id_t
    case ROCPROFILER_BUFFER_TRACING_MARKER_CORE_API:
      // see rocprofiler_marker_core_api_id_t
    case ROCPROFILER_BUFFER_TRACING_MARKER_CONTROL_API:
      // see rocprofiler_marker_control_api_id_t
    case ROCPROFILER_BUFFER_TRACING_MARKER_NAME_API:
      // see rocprofiler_marker_name_api_id_t

    default:
      convert_unknown(ga);
#if 0
      PRINT("rocprofiler buffer event: Unhandled activity: category %u, kind %u\n",
        header->category, header->kind);

#endif
      break;
  }

  return cid;
}


static uint64_t
rocm_activity_translate_pc_sampling
(
  gpu_activity_t *ga,
  rocprofiler_record_header_t *header
)
{
  uint64_t cid = 0;

  switch (header->kind) {
    case ROCPROFILER_PC_SAMPLING_RECORD_HOST: {
      DECL_INIT_CAST(rocprofiler_pc_sampling_record_sw_t *, pcs,
                     header->payload);
      cid = convert_pc_sampling_sw(ga, pcs);
      break;
    }
#ifdef ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC
    case ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC: {
      DECL_INIT_CAST(rocprofiler_pc_sampling_record_hw_t *, pcs,
                     header->payload);
      cid = convert_pc_sampling_hw(ga, pcs);
      break;
    }
#endif
#ifdef ROCPROFILER_PC_SAMPLING_RECORD_INVALID
    case ROCPROFILER_PC_SAMPLING_RECORD_INVALID_SAMPLE:
      break;
#endif
#ifdef ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_LOAD_MARKER
    case ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_LOAD_MARKER:
    case ROCPROFILER_PC_SAMPLING_RECORD_CODE_OBJECT_UNLOAD_MARKER:
      // ROCm 6.2.0 only; unused but must be recognized if present
      // to avoid default error case below
      break;
#endif
    default:
      PRINT("PC sampling: unexpected record kind: %d\n", header->kind);
      exit(-1);
  }

  return cid;
}


static uint64_t
rocm_activity_translate
(
  gpu_activity_t *ga,
  rocprofiler_record_header_t *header
)
{
  uint64_t cid = 0;

  gpu_activity_init(ga);

  switch(header->category) {
    case ROCPROFILER_BUFFER_CATEGORY_TRACING:
      cid = rocm_activity_translate_buffer_tracing(ga, header);
      break;

    case ROCPROFILER_BUFFER_CATEGORY_PC_SAMPLING:
      cid = rocm_activity_translate_pc_sampling(ga, header);
      break;

    default:
      PRINT("rocm_activity_translate: unexpected record category: %d\n",
            header->category);
  }

  cstack_ptr_set(&(ga->next), 0);

  return cid;
}


#if TEST_SCRATCH
static void
fake_scratch_record
(
  rocprofiler_buffer_tracing_scratch_memory_record_t *activity,
  rocprofiler_scratch_memory_operation_t op
)
{
  activity->kind = ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY;
  activity->start_timestamp = 0;
  activity->end_timestamp = 1000000000;
  activity->correlation_id.external.value = GPU_RUNTIME_PH_CID;
  activity->operation = op;
}


static void
send_fake_scratch_activities
(
  void
)
{
  static int tested = 0;
  if (!tested) {
    uint64_t correlation_id = GPU_RUNTIME_PH_CID;
    gpu_activity_t scratch;
    rocprofiler_buffer_tracing_scratch_memory_record_t foo;
    rocprofiler_record_header_t header;

    header.category = ROCPROFILER_BUFFER_CATEGORY_TRACING;
    header.kind = ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY;
    header.payload = &foo;

    fake_scratch_record(&foo, ROCPROFILER_SCRATCH_MEMORY_ALLOC);
    rocm_activity_translate(&scratch, &header);
    rocm_activity_send(correlation_id, &scratch);

    fake_scratch_record(&foo, ROCPROFILER_SCRATCH_MEMORY_FREE);
    rocm_activity_translate(&scratch, &header);
    rocm_activity_send(correlation_id, &scratch);

    fake_scratch_record(&foo, ROCPROFILER_SCRATCH_MEMORY_ASYNC_RECLAIM);
    rocm_activity_translate(&scratch, &header);
    rocm_activity_send(correlation_id, &scratch);

    tested = 1;
  }
}
#endif


//******************************************************************************
// interface operations
//******************************************************************************

void
rocm_activity_send
(
  uint64_t correlation_id,
  gpu_activity_t *gpu_activity
)
{
  uint32_t thread_id =
    gpu_activity_channel_correlation_id_get_thread_id(correlation_id);

  gpu_activity_channel_t *channel = gpu_activity_channel_lookup(thread_id);

  if (channel == NULL) {
    TMSG(ROCM, "Cannot find gpu_activity_channel "
              "(correlation ID: %" PRIu64 ")", correlation_id);
    return;
  }

  PRINT("rocm_activity_send: sending activity kind(%d)=%s to thread %d (extid=0x%lx)\n",
        gpu_activity->kind, gpu_activity_kind_to_string(gpu_activity->kind),
        thread_id, correlation_id);

  gpu_activity_channel_send(channel, gpu_activity);
}


uint64_t
rocm_activity_process
(
 rocprofiler_record_header_t *rocprofiler_record
)
{
  if (ENABLED(ROCM)) {
    uint32_t category = rocprofiler_record->category;
    uint32_t kind = rocprofiler_record->kind;
    uint64_t extid = rocm_get_extern_id(rocprofiler_record);
    const char *name = NULL;

    if (category == ROCPROFILER_BUFFER_CATEGORY_TRACING)  {
      name = rocm_get_buffer_kind_name(kind);
      TMSG(ROCM, "rocm_activity_process: TRACING kind(%d)=%s  extid: 0x%lx",
           kind, name, extid);
    } else if (category == ROCPROFILER_BUFFER_CATEGORY_PC_SAMPLING) {
      name = rocm_get_pc_sampling_kind_name(kind);
      TMSG(ROCM, "rocm_activity_process: PC_SAMPLING kind(%d)=%s", kind, name);
    } else if (category == ROCPROFILER_BUFFER_CATEGORY_COUNTERS) {
      TMSG(ROCM, "rocm_activity_process: COUNTERS");
    } else {
      TMSG(ROCM, "rocm_activity_process: UNKNOWN category");
    }
  }

  gpu_activity_t gpu_activity;

  memset(&gpu_activity, 0, sizeof(gpu_activity));
  gpu_activity.kind = GPU_ACTIVITY_UNKNOWN;

  uint64_t correlation_id =
    rocm_activity_translate(&gpu_activity, rocprofiler_record);

  if (gpu_activity.kind == GPU_ACTIVITY_UNKNOWN) return 0;

  rocm_activity_send(correlation_id, &gpu_activity);

#if TEST_SCRATCH
  send_fake_scratch_activities();
#endif
  return correlation_id;
}
