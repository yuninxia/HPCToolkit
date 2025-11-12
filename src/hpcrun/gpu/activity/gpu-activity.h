// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_activity_h
#define gpu_activity_h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>

//******************************************************************************
// local includes
//******************************************************************************

#include "../../../common/lean/stacks.h"
#include "../../utilities/ip-normalized.h"

#ifndef __cplusplus
#include <stdatomic.h>
#else
#include <atomic>
#endif


//******************************************************************************
// forward declarations
//******************************************************************************

typedef struct cct_node_t cct_node_t;



//******************************************************************************
// type declarations
//******************************************************************************

typedef enum {
  GPU_ACTIVITY_UNKNOWN = 0,
  GPU_ACTIVITY_KERNEL = 1,
  GPU_ACTIVITY_KERNEL_BLOCK = 2,
  GPU_ACTIVITY_MEMCPY = 3,
  GPU_ACTIVITY_MEMCPY2 = 4,
  GPU_ACTIVITY_MEMSET = 5,
  GPU_ACTIVITY_MEMORY = 6,
  GPU_ACTIVITY_SYNCHRONIZATION = 7,
  GPU_ACTIVITY_GLOBAL_ACCESS = 8,
  GPU_ACTIVITY_LOCAL_ACCESS = 9,
  GPU_ACTIVITY_BRANCH = 10,
  GPU_ACTIVITY_CDP_KERNEL = 11,
  GPU_ACTIVITY_PC_SAMPLING = 12,
  GPU_ACTIVITY_PC_SAMPLING_INFO = 13,
  GPU_ACTIVITY_EVENT = 14,
  GPU_ACTIVITY_FLUSH = 15,
  GPU_ACTIVITY_COUNTERS = 16,
  GPU_ACTIVITY_INTEL_OPTIMIZATION = 17,
  GPU_ACTIVITY_BLAME_SHIFT = 18,
  GPU_ACTIVITY_INTEL_GPU_UTILIZATION = 19,
  GPU_ACTIVITY_KERNEL_SIMD_GROUP = 20,
  GPU_ACTIVITY_ONE_COUNTER = 21,
  GPU_ACTIVITY_SCRATCH = 22,
  GPU_ACTIVITY_PAGE_MIGRATION = 23,
} gpu_activity_kind_t;

typedef enum {
  GPU_ENTRY_TYPE_ACTIVITY = 1,
  GPU_ENTRY_TYPE_NOTIFICATION = 2,
  GPU_ENTRY_TYPE_BUFFER = 3,
  GPU_ENTRY_TYPE_COUNT = 4
} gpu_entry_type_t;

// values are used to index within GMEM metric kinds
typedef enum {
  GPU_GLOBAL_ACCESS_LOAD_CACHED = 0,
  GPU_GLOBAL_ACCESS_LOAD_UNCACHED = 1,
  GPU_GLOBAL_ACCESS_STORE = 2,
  GPU_GLOBAL_ACCESS_COUNT = 3
} gpu_global_access_type_t;

typedef enum {
  GPU_LOCAL_ACCESS_LOAD,
  GPU_LOCAL_ACCESS_STORE,
  GPU_LOCAL_ACCESS_COUNT
} gpu_local_access_type_t;

typedef enum {
  GPU_SYNC_UNKNOWN = 0,
  GPU_SYNC_EVENT = 1,
  GPU_SYNC_STREAM_EVENT_WAIT = 2,
  GPU_SYNC_STREAM = 3,
  GPU_SYNC_CONTEXT = 4,
  GPU_SYNC_COUNT = 5
} gpu_sync_type_t;

typedef enum {
  GPU_MEMCPY_UNK = 0,  // unknown
  GPU_MEMCPY_H2D = 1,  // host to device
  GPU_MEMCPY_D2H = 2,  // device to host
  GPU_MEMCPY_H2A = 3,  // host to device array
  GPU_MEMCPY_A2H = 4,  // device array to host
  GPU_MEMCPY_A2A = 5,  // device array to device array
  GPU_MEMCPY_A2D = 6,  // device array to device
  GPU_MEMCPY_D2A = 7,  // device to device array
  GPU_MEMCPY_D2D = 8,  // device to device
  GPU_MEMCPY_H2H = 9,  // host to host
  GPU_MEMCPY_P2P = 10, // device peer to device peer
  GPU_MEMCPY_COUNT = 11
} gpu_memcpy_type_t;

typedef enum {
  GPU_INST_STALL_NONE = 1,
  GPU_INST_STALL_IFETCH = 2,
  GPU_INST_STALL_IDEPEND = 3,
  GPU_INST_STALL_MEM = 4,
  GPU_INST_STALL_GMEM = 5,
  GPU_INST_STALL_TMEM = 6,
  GPU_INST_STALL_SYNC = 7,
  GPU_INST_STALL_CMEM = 8,
  GPU_INST_STALL_PIPE_BUSY = 9,
  GPU_INST_STALL_MEM_THROTTLE = 10,
  GPU_INST_STALL_NOT_SELECTED = 11,
  GPU_INST_STALL_OTHER = 12,
  GPU_INST_STALL_SLEEP = 13,
  GPU_INST_STALL_DONTCARE = 14,
  GPU_INST_STALL_INVALID = 15
} gpu_inst_stall_t;

typedef enum {
  GPU_INST_TYPE_NONE = 0,
  GPU_INST_TYPE_VECTOR_DUAL, // 2 simple 32-bit instructions, no dep --> main + limited secondary units
  GPU_INST_TYPE_VECTOR,
  GPU_INST_TYPE_MATRIX,
  GPU_INST_TYPE_SCALAR,
  GPU_INST_TYPE_TEXTURE, // texture sample or image_load
  GPU_INST_TYPE_LDS,       // AMD: Local Data Share (LDS); NVIDIA: shared memory;
  GPU_INST_TYPE_LDS_DIRECT, // global --> LDS, permute?, swizzle?
  GPU_INST_TYPE_FLAT, // access to: global memory, LDS, scratch (spill area), host memory
  GPU_INST_TYPE_EXPORT, // graphics only: to transmit final pixel or vectex data
  GPU_INST_TYPE_MSG, // graphics pipeline control or interrupt CPU
  GPU_INST_TYPE_BARRIER, // wait for all threads in workgroup
  GPU_INST_TYPE_BRANCH_TAKEN,
  GPU_INST_TYPE_BRANCH_NOT_TAKEN,
  GPU_INST_TYPE_JUMP,
  GPU_INST_TYPE_OTHER,
  GPU_INST_TYPE_ISSUED,
  GPU_INST_TYPE_UNUSED // NOTE: this must be last because it has no metric
} gpu_inst_type_t;

typedef enum {
  GPU_UTIL_METRICS_WAVE_ACT = 0,
  GPU_UTIL_METRICS_WAVE_AVL = 1,
  GPU_UTIL_METRICS_WAVE_UTL = 2,
  GPU_UTIL_METRICS_THR_ACT = 3,
  GPU_UTIL_METRICS_THR_AVL = 4,
  GPU_UTIL_METRICS_THR_UTL = 5,
  GPU_UTIL_METRICS_LAST = 6
} gpu_inst_metrics_t;

#define FORALL_AMD_GPU_PIPES(macro)                                                                  \
  macro(GPU_PIPE_TYPE_VECTOR_DUAL, vector_dual_alu) /* 2 simple 32-bit VALU ops, no dep           */ \
  macro(GPU_PIPE_TYPE_VECTOR,      vector_alu)      /* full VALU instructions                     */ \
  macro(GPU_PIPE_TYPE_MATRIX,      matrix)          /* matrix instructions                        */ \
  macro(GPU_PIPE_TYPE_SCALAR,      scalar)          /* SALU or SMEM instructions                  */ \
  macro(GPU_PIPE_TYPE_LDS,         lds)             /* LDS instructions                           */ \
  macro(GPU_PIPE_TYPE_LDS_DIRECT,  lds_direct)      /* LDS direct instructions                    */ \
  macro(GPU_PIPE_TYPE_TEXTURE,     texture)         /* texture sample or image_load               */ \
  macro(GPU_PIPE_TYPE_FLAT,        flat)            /* mem access: global, LDS, scratch, host     */ \
  macro(GPU_PIPE_TYPE_EXPORT,      export)          /* graphics only: xmit final pixel or vertex  */ \
  macro(GPU_PIPE_TYPE_BRMSG,       branch_msg)      /* graphics pipeline control or interrupt CPU */ \
  macro(GPU_PIPE_TYPE_MISC,        misc)            /* miscellaneous instructions                 */

#define DECL_ENUM(enum, field) enum,
typedef enum {
  FORALL_AMD_GPU_PIPES(DECL_ENUM)
  GPU_PIPE_TYPE_LAST
} gpu_pipeline_type_t;

typedef enum {
  GPU_MEM_ARRAY = 0,
  GPU_MEM_DEVICE = 1,
  GPU_MEM_MANAGED = 2,
  GPU_MEM_PAGEABLE = 3,
  GPU_MEM_PINNED = 4,
  GPU_MEM_DEVICE_STATIC = 5,
  GPU_MEM_MANAGED_STATIC = 6,
  GPU_MEM_UNKNOWN = 7,
  GPU_MEM_COUNT = 8
} gpu_mem_type_t;

typedef enum {
  INORDER_QUEUE = 0,
  KERNEL_TO_MULTIPLE_CONTEXTS = 1,
  KERNEL_PARAMS_NOT_ALIASED = 2,
  KERNEL_PARAMS_ALIASED = 3,
  SINGLE_DEVICE_USE_AOT_COMPILATION = 4,
  OUTPUT_OF_KERNEL_INPUT_TO_ANOTHER_KERNEL = 5,
  UNUSED_DEVICES = 6
} intel_optimization_type_t;

typedef enum {
  GPU_MEM_OP_ALLOC = 0,
  GPU_MEM_OP_DELETE = 1,
  GPU_MEM_OP_UNKNOWN = 2
} gpu_mem_op_t;

typedef enum {
  GPU_PAGE_NONE,
  GPU_PAGE_MIGRATE_START,
  GPU_PAGE_MIGRATE_END,
  GPU_PAGE_FAULT_START,
  GPU_PAGE_FAULT_END,
  GPU_PAGE_QUEUE_EVICTION,
  GPU_PAGE_QUEUE_RESTORE,
  GPU_PAGE_UNMAP,
  GPU_PAGE_DROPPED_EVENT,
} gpu_page_op_type_t;

// Note: TTM = Linux Translation Table Manager
typedef enum {
  GPU_PAGE_TRIGGER_NONE,
  GPU_PAGE_TRIGGER_NOTFOUND,

  GPU_PAGE_MIGRATE_TRIGGER_UNKNOWN,
  GPU_PAGE_MIGRATE_PREFETCH,
  GPU_PAGE_MIGRATE_FAULT_GPU,
  GPU_PAGE_MIGRATE_FAULT_CPU,
  GPU_PAGE_MIGRATE_TTM_EVICTION,

  GPU_PAGE_QUEUE_SUSPEND_TRIGGER_UNKNOWN,
  GPU_PAGE_QUEUE_SUSPEND_SVM,
  GPU_PAGE_QUEUE_SUSPEND_USERPTR,
  GPU_PAGE_QUEUE_SUSPEND_TTM,
  GPU_PAGE_QUEUE_SUSPEND_SUSPEND,

  GPU_PAGE_UNMAP_TRIGGER_UNKNOWN,
  GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY,
  GPU_PAGE_UNMAP_FROM_GPU_MMU_NOTIFY_MIGRATE,
  GPU_PAGE_UNMAP_FROM_CPU,

} gpu_page_op_trigger_t;

typedef enum {
  GPU_SCRATCH_MEMORY_ALLOC,
  GPU_SCRATCH_MEMORY_FREE,
  GPU_SCRATCH_MEMORY_ASYNC_RECLAIM,
  GPU_SCRATCH_MEMORY_ILLEGAL
} gpu_scratch_op_t;

#define BITFIELD16(field) uint16_t field : 1;
#define DECL_BITFIELD(enum, field) BITFIELD16(field)
typedef struct gpu_pipeline_status_t {
  BITFIELD16(available)    // info is available about pipelines
  FORALL_AMD_GPU_PIPES(DECL_BITFIELD)
} gpu_pipeline_status_t;

typedef struct gpu_pipeline_info_t {
  gpu_pipeline_status_t issued;
  gpu_pipeline_status_t stalled;
} gpu_pipeline_info_t;

// pc sampling
typedef struct gpu_pc_sampling_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
  uint32_t issues;
  uint32_t non_issues;
  gpu_inst_stall_t issue_stall_reason;
  bool issue_stall_exposed;
  gpu_inst_type_t inst_type;
  gpu_pipeline_info_t pipeline_info;
  uint32_t waves_active;
  uint32_t waves_available;
  uint32_t threads_active;
  uint32_t threads_available;
} gpu_pc_sampling_t;

typedef struct gpu_pc_sampling_info_t {
  uint64_t correlation_id;
  uint64_t droppedSamples;
  uint64_t samplingPeriodInCycles;
  uint64_t totalSamples;
  uint64_t fullSMSamples;
} gpu_pc_sampling_info_t;

// a special flush record to notify all operations have been consumed
typedef struct gpu_flush_t {
#ifdef __cplusplus
  std::
#endif
  atomic_bool *wait;
} gpu_flush_t;

// this type is prefix of all memory structures
// gpu_interval_t is a prefix
typedef struct gpu_mem_t {
  uint64_t start;
  uint64_t end;
  uint64_t bytes;
} gpu_mem_t;

// gpu_mem_t is a prefix
typedef struct gpu_memcpy_t {
  uint64_t start;
  uint64_t end;
  uint64_t bytes;
  uint64_t submit_time;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
  gpu_memcpy_type_t copyKind;
} gpu_memcpy_t;

// memory allocation or free operation
// gpu_mem_t is a prefix
typedef struct gpu_memory_t {
  uint64_t start;
  uint64_t end;
  uint64_t bytes;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
  gpu_mem_type_t memKind;
  gpu_mem_op_t mem_op;
} gpu_memory_t;

// gpu_mem_t is a prefix
typedef struct gpu_memset_t {
  uint64_t start;
  uint64_t end;
  uint64_t bytes;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
  gpu_mem_type_t memKind;
} gpu_memset_t;

// gpu kernel execution
typedef struct gpu_kernel_t {
  uint64_t start;
  uint64_t end;
  uint64_t submit_time;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
  ip_normalized_t kernel_first_pc;
  int32_t dynamicSharedMemory;
  int32_t staticSharedMemory;
  int32_t localMemoryTotal;
  uint32_t activeWarpsPerSM;
  uint32_t maxActiveWarpsPerSM;
  uint32_t scalarRegisters;
  uint32_t vectorRegisters;
  uint32_t blocks;
  uint32_t blockThreads;
  uint32_t blockSharedMemory;
} gpu_kernel_t;

typedef struct gpu_kernel_block_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
  uint64_t execution_count;
  uint64_t latency;
  uint64_t active_simd_lanes;
} gpu_kernel_block_t;

typedef struct gpu_cdpkernel_t {
  uint64_t start;
  uint64_t end;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
} gpu_cdpkernel_t;

typedef struct gpu_function_t {
  uint32_t function_id;
  ip_normalized_t pc;
} gpu_function_t;

typedef struct gpu_event_t {
  uint64_t correlation_id;
  uint32_t event_id;
  uint32_t context_id;
  uint32_t stream_id;
} gpu_event_t;

typedef struct gpu_global_access_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
  uint64_t l2_transactions;
  uint64_t theoreticalL2Transactions;
  uint64_t bytes;
  gpu_global_access_type_t type;
} gpu_global_access_t;

typedef struct gpu_local_access_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
  uint64_t sharedTransactions;
  uint64_t theoreticalSharedTransactions;
  uint64_t bytes;
  gpu_local_access_type_t type;
} gpu_local_access_t;

typedef struct gpu_branch_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
  uint32_t diverged;
  uint32_t executed;
} gpu_branch_t;

typedef struct gpu_blame_shift_t {
  double cpu_idle_time;
  double gpu_idle_cause_time;
  double cpu_idle_cause_time;
} gpu_blame_shift_t;

typedef struct gpu_synchronization_t {
  uint64_t start;
  uint64_t end;
  uint64_t correlation_id;
  uint32_t context_id;
  uint32_t stream_id;
  uint32_t event_id;
  gpu_sync_type_t syncKind;
} gpu_synchronization_t;

typedef struct gpu_page_migration_t {
  uint64_t timestamp;
  gpu_page_op_type_t op_type;
  gpu_page_op_trigger_t trigger;
  uint64_t start_addr;
  uint64_t end_addr;
  uint64_t bytes;
  uint32_t dropped_events_count;
  uint64_t correlation_id;
} gpu_page_migration_t;

typedef struct gpu_host_correlation_t {
  uint64_t correlation_id;
  uint64_t host_correlation_id;
} gpu_host_correlation_t;

typedef double gpu_counter_value_t;

typedef struct gpu_counter_t {
  uint64_t correlation_id;
  ip_normalized_t kernel_first_pc;
  int total_counters;
  // The function that creates the structure should
  // be responsible for allocating memory.
  // The function that attributes the structure should
  // be responsible for deallocating the memory.
  gpu_counter_value_t *values;
} gpu_counter_t;

typedef struct gpu_one_counter_t {
  uint64_t correlation_id;
  ip_normalized_t kernel_first_pc;
  unsigned int metric_id;
  gpu_counter_value_t value;
} gpu_one_counter_t;

// a type that can be used to access start and end times
// for a subset of activity kinds including kernel execution,
// memcpy, memset,
typedef struct gpu_interval_t {
  uint64_t start;
  uint64_t end;
} gpu_interval_t;

typedef struct gpu_instruction_t {
  uint64_t correlation_id;
  ip_normalized_t pc;
} gpu_instruction_t;

typedef struct intel_optimization_t {
  uint32_t val;
  intel_optimization_type_t intelOptKind;
} intel_optimization_t;

typedef struct gpu_utilization_t {
  uint8_t active;
  uint8_t stalled;
  uint8_t idle;
} gpu_utilization_t;

// gpu_mem_t is a prefix
typedef struct gpu_scratch_t {
  uint64_t start;
  uint64_t end;
  gpu_scratch_op_t op_type;
  uint64_t correlation_id;
  uint32_t device_id;
  uint32_t context_id;
  uint32_t stream_id;
} gpu_scratch_t;

typedef struct gpu_activity_details_t {
  union {
    // Each field stores the complete information needed
    // for processing each activity kind.
    gpu_pc_sampling_t pc_sampling;
    gpu_pc_sampling_info_t pc_sampling_info;
    gpu_memcpy_t memcpy;
    gpu_memory_t memory;
    gpu_memset_t memset;
    gpu_kernel_t kernel;
    gpu_kernel_block_t kernel_block;
    gpu_function_t function;
    gpu_cdpkernel_t cdpkernel;
    gpu_event_t event;
    gpu_global_access_t global_access;
    gpu_local_access_t local_access;
    gpu_branch_t branch;
    gpu_synchronization_t synchronization;
    gpu_host_correlation_t correlation;
    gpu_flush_t flush;
    gpu_counter_t counters;
    gpu_one_counter_t counter;
    intel_optimization_t intel_optimization;
    gpu_blame_shift_t blame_shift;
    gpu_utilization_t gpu_utilization_info;
    gpu_scratch_t scratch;
    gpu_page_migration_t migration;

    // Access short cuts for activity fields shared by multiple kinds

    // Used to access start time and end time of a coarse grained activity.
    gpu_interval_t interval;

    // Information about specific instructions in fine-grained measurements
    gpu_instruction_t instruction;
  };
} gpu_activity_details_t;

typedef struct gpu_activity_t {
  s_element_ptr_t next;
  gpu_activity_kind_t kind;
  gpu_activity_details_t details;
  cct_node_t *cct_node;
} gpu_activity_t;

typedef void (*gpu_activity_attribute_fn_t)(gpu_activity_t *a);

//******************************************************************************
// interface functions
//******************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

void
gpu_activity_init
(
 gpu_activity_t *activity
);

void gpu_interval_set
(
 gpu_interval_t *interval,
 uint64_t start,
 uint64_t end
);

bool
gpu_interval_is_invalid
(
  gpu_interval_t *gi
);

void gpu_context_activity_dump
(
 const gpu_activity_t *activity,
 const char *context
);

const char *
gpu_activity_kind_to_string
(
 gpu_activity_kind_t kind
);

const char *
gpu_mem_type_to_string
(
 gpu_mem_type_t type
);

const char *
gpu_page_op_type_to_string
(
  gpu_page_op_type_t type
);

const char *
gpu_page_trigger_to_string
(
  gpu_page_op_trigger_t type
);

#ifdef __cplusplus
}
#endif

#endif
