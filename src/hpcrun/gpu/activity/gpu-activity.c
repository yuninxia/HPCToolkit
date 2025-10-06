// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../messages/messages.h"

#include "gpu-activity.h"



//******************************************************************************
// macros
//******************************************************************************

#define GPU_INTERVAL_ENDPOINT_INVALID 0

#define GPU_INTERVAL_ENDPOINT_ISINVALID(time) \
  ((time) == GPU_INTERVAL_ENDPOINT_INVALID)


#define FORALL_ACTIVITY_KINDS(macro)                                      \
  macro(GPU_ACTIVITY_UNKNOWN)                                             \
  macro(GPU_ACTIVITY_KERNEL)                                              \
  macro(GPU_ACTIVITY_KERNEL_BLOCK)                                        \
  macro(GPU_ACTIVITY_MEMCPY)                                              \
  macro(GPU_ACTIVITY_MEMCPY2)                                             \
  macro(GPU_ACTIVITY_MEMSET)                                              \
  macro(GPU_ACTIVITY_MEMORY)                                              \
  macro(GPU_ACTIVITY_SYNCHRONIZATION)                                     \
  macro(GPU_ACTIVITY_GLOBAL_ACCESS)                                       \
  macro(GPU_ACTIVITY_LOCAL_ACCESS)                                        \
  macro(GPU_ACTIVITY_BRANCH)                                              \
  macro(GPU_ACTIVITY_CDP_KERNEL)                                          \
  macro(GPU_ACTIVITY_PC_SAMPLING)                                         \
  macro(GPU_ACTIVITY_PC_SAMPLING_INFO)                                    \
  macro(GPU_ACTIVITY_EVENT)                                               \
  macro(GPU_ACTIVITY_FLUSH)                                               \
  macro(GPU_ACTIVITY_COUNTERS)                                            \
  macro(GPU_ACTIVITY_INTEL_OPTIMIZATION)                                  \
  macro(GPU_ACTIVITY_BLAME_SHIFT)                                         \
  macro(GPU_ACTIVITY_INTEL_GPU_UTILIZATION)                               \
  macro(GPU_ACTIVITY_KERNEL_SIMD_GROUP)                                   \
  macro(GPU_ACTIVITY_ONE_COUNTER)                                         \
  macro(GPU_ACTIVITY_PAGE_MIGRATION)                                      \
  macro(GPU_ACTIVITY_SCRATCH)

#define FORALL_MEM_TYPES(macro)                                           \
  macro(GPU_MEM_ARRAY)                                                    \
  macro(GPU_MEM_DEVICE)                                                   \
  macro(GPU_MEM_MANAGED)                                                  \
  macro(GPU_MEM_PAGEABLE)                                                 \
  macro(GPU_MEM_PINNED)                                                   \
  macro(GPU_MEM_DEVICE_STATIC)                                            \
  macro(GPU_MEM_MANAGED_STATIC)                                           \
  macro(GPU_MEM_UNKNOWN)

#define FORALL_PAGE_OP_TYPES(macro)                                       \
  macro(GPU_PAGE_NONE)                                                    \
  macro(GPU_PAGE_MIGRATE_START)                                           \
  macro(GPU_PAGE_MIGRATE_END)                                             \
  macro(GPU_PAGE_FAULT_START)                                             \
  macro(GPU_PAGE_FAULT_END)                                               \
  macro(GPU_PAGE_QUEUE_EVICTION)                                          \
  macro(GPU_PAGE_QUEUE_RESTORE)                                           \
  macro(GPU_PAGE_UNMAP)                                                   \
  macro(GPU_PAGE_DROPPED_EVENT)

#define FORALL_PAGE_TRIGGER_TYPES(macro)                                  \
  macro(GPU_PAGE_TRIGGER_NONE)                                            \
  macro(GPU_PAGE_TRIGGER_NOTFOUND)                                        \
  macro(GPU_PAGE_MIGRATE_TRIGGER_UNKNOWN)                                 \
  macro(GPU_PAGE_MIGRATE_PREFETCH)                                        \
  macro(GPU_PAGE_MIGRATE_FAULT_GPU)                                       \
  macro(GPU_PAGE_MIGRATE_FAULT_CPU)                                       \
  macro(GPU_PAGE_MIGRATE_TTM_EVICTION)                                    \
  macro(GPU_PAGE_QUEUE_SUSPEND_TRIGGER_UNKNOWN)                           \
  macro(GPU_PAGE_QUEUE_SUSPEND_SVM)                                       \
  macro(GPU_PAGE_QUEUE_SUSPEND_USERPTR)                                   \
  macro(GPU_PAGE_QUEUE_SUSPEND_TTM)                                       \
  macro(GPU_PAGE_QUEUE_SUSPEND_SUSPEND)                                   \
  macro(GPU_PAGE_UNMAP_TRIGGER_UNKNOWN)                                   \
  macro(GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY)                               \
  macro(GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY_MIGRATE)                       \
  macro(GPU_PAGE_UNMAP_FROM_CPU)


#define CODE_TO_STRING(e) case e: return #e;


//******************************************************************************
// interface functions
//******************************************************************************

#define DEBUG 0

#include "../common/gpu-print.h"



//******************************************************************************
// interface functions
//******************************************************************************

void
gpu_activity_init
(
 gpu_activity_t *activity
)
{
  memset(activity, 0, sizeof(gpu_activity_t));
}


void
gpu_context_activity_dump
(
 const gpu_activity_t *activity,
 const char *context
)
{
  PRINT("context %s gpu activity %p kind = %d\n", context, activity, activity->kind);
}


void
gpu_activity_dump
(
 const gpu_activity_t *activity
)
{
  gpu_context_activity_dump(activity, "DEBUGGER");
}


void
gpu_interval_set
(
 gpu_interval_t* interval,
 uint64_t start,
 uint64_t end
)
{
  if (start > end) {
    EMSG("WARNING: Suppressing reversed time interval for GPU activity: %u > %u",
      (unsigned long)start, (unsigned long)end);
    end = start = GPU_INTERVAL_ENDPOINT_INVALID;
  }

  interval->start = start;
  interval->end = end;
  PRINT("gpu interval: [%lu, %lu) delta = %ld\n", interval->start,
        interval->end, interval->end - interval->start);
}


bool
gpu_interval_is_invalid
(
  gpu_interval_t *gi
)
{
  return GPU_INTERVAL_ENDPOINT_ISINVALID(gi->start) |
         GPU_INTERVAL_ENDPOINT_ISINVALID(gi->end);
}


const char*
gpu_activity_kind_to_string
(
  gpu_activity_kind_t kind
)
{
  switch (kind) {
    FORALL_ACTIVITY_KINDS(CODE_TO_STRING)
    default: return "GPU_ACTIVITY_NOTFOUND";
  }
}


const char *
gpu_mem_type_to_string
(
  gpu_mem_type_t type
)
{
  switch (type) {
    FORALL_MEM_TYPES(CODE_TO_STRING)
    default: return "GPU_MEM_NOTFOUND";
  }
}


const char *
gpu_page_op_type_to_string
(
  gpu_page_op_type_t type
)
{
  switch (type) {
    FORALL_PAGE_OP_TYPES(CODE_TO_STRING)
    default: return "GPU_PAGE_NOTFOUND";
  }
}


const char *
gpu_page_trigger_to_string
(
  gpu_page_op_trigger_t type
)
{
  switch (type){
    FORALL_PAGE_TRIGGER_TYPES(CODE_TO_STRING)
    default: return "GPU_PAGE_TRIGGER_NOTFOUND";
  }
}
