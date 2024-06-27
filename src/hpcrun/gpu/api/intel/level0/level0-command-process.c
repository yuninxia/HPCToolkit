// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// local includes
//*****************************************************************************

#define _GNU_SOURCE

#include "level0-command-process.h"
#include "level0-data-node.h"
#include "level0-binary.h"
#include "level0-kernel-module-map.h"
#include "level0-api.h"

#include "../../../activity/gpu-activity-channel.h"
#include "../../../activity/gpu-activity-process.h"
#include "../../../activity/correlation/gpu-correlation-channel.h"
#include "../../../api/nvidia/cuda-correlation-id-map.h"
#include "../../../activity/correlation/gpu-host-correlation-map.h"
#include "../../../activity/gpu-op-ccts-map.h"
#include "../../../gpu-monitoring-thread-api.h"
#include "../../../gpu-application-thread-api.h"
#include "../../../activity/gpu-op-placeholders.h"
#include "../../../operation/gpu-operation-multiplexer.h"
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



#include "../../../common/gpu-print.h"

//*****************************************************************************
// global variables
//*****************************************************************************

pthread_mutex_t gpu_activity_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
bool data_processed = false;
ze_event_handle_t kernel_event = NULL;

//*****************************************************************************
// local variables
//*****************************************************************************

// The thread itself how many pending operations
static __thread atomic_int level0_self_pending_operations = 0;

//*****************************************************************************
// private operations
//*****************************************************************************

static void 
level0_pcsamples_sync
(
  level0_data_node_t* command_node, 
  pthread_mutex_t* mtx, 
  pthread_cond_t* cv, 
  bool* processed_flag
) 
{
  if (command_node && command_node->type == LEVEL0_KERNEL) {
    // Notify the profile thread to process the data
    kernel_event = command_node->event;

    // Lock until data is processed
    pthread_mutex_lock(mtx);
    while (!(*processed_flag)) {
        pthread_cond_wait(cv, mtx);
    }
    pthread_mutex_unlock(mtx);

    // Reset the flag
    pthread_mutex_lock(mtx);
    *processed_flag = false;
    pthread_mutex_unlock(mtx);
  }
}

static void
level0_kernel_translate
(
  gpu_activity_t* ga,
  level0_data_node_t* c,
  uint64_t start,
  uint64_t end
)
{
  PRINT("level0_kernel_translate: submit_time %llu, start %llu, end %llu\n", c->submit_time, start, end);
  ga->kind = GPU_ACTIVITY_KERNEL;
  ga->details.kernel.kernel_first_pc = ip_normalized_NULL;
  if (level0_pcsampling_enabled()) {
    ga->details.kernel.correlation_id = c->correlation_id;
  } else {
    ga->details.kernel.correlation_id = (uint64_t)(c->event);
  }
  ga->details.kernel.submit_time = c->submit_time;
  gpu_interval_set(&ga->details.interval, start, end);
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
  PRINT("level0_memcpy_translate: src_type %d, dst_type %d, size %u\n",
    c->details.memcpy.src_type,
    c->details.memcpy.dst_type,
    c->details.memcpy.copy_size);

  ga->kind = GPU_ACTIVITY_MEMCPY;
  ga->details.memcpy.bytes = c->details.memcpy.copy_size;
  if (level0_pcsampling_enabled()) {
    ga->details.memcpy.correlation_id = c->correlation_id;
  } else {
    ga->details.memcpy.correlation_id = (uint64_t)(c->event);
  }
  ga->details.memcpy.submit_time = c->submit_time;

  // Switch on memory src and dst types
  ga->details.memcpy.copyKind = GPU_MEMCPY_UNK;
  switch (c->details.memcpy.src_type) {
    case ZE_MEMORY_TYPE_HOST: {
      switch (c->details.memcpy.dst_type) {
        case ZE_MEMORY_TYPE_HOST:
          ga->details.memcpy.copyKind = GPU_MEMCPY_H2H;
          break;
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
    default:
      break;
  }
  gpu_interval_set(&ga->details.interval, start, end);
}

static uint32_t
get_load_module
(
  ze_kernel_handle_t kernel
)
{
  // Step 1: get the string name for the kernel
  size_t name_size = 0;
  zeKernelGetName(kernel, &name_size, NULL);
  char* kernel_name = malloc(name_size);
  zeKernelGetName(kernel, &name_size, kernel_name);

  // Compute hash string for the kernel name
  char kernel_name_hash[CRYPTO_HASH_STRING_LENGTH];
  crypto_compute_hash_string(kernel_name, strlen(kernel_name), kernel_name_hash, CRYPTO_HASH_STRING_LENGTH);

  free(kernel_name);

  // Step 2: get the hash for the binary that contains the kernel
  const char *binary_hash;
  gpu_binary_kind_t bkind;

  ze_module_handle_t module_handle = level0_kernel_module_map_lookup(kernel);
  PRINT("get_load_module: kernel handle %p, module handle %p\n", kernel, module_handle);
  level0_module_handle_map_lookup(module_handle, &binary_hash, &bkind);
  //
  // Step 3: generate <binary hash>.gpubin as the kernel load module name
  char load_module_name[PATH_MAX] = {'\0'};
  char load_module_name_fullpath[PATH_MAX] = {'\0'};
  gpu_binary_path_generate(binary_hash, load_module_name, load_module_name_fullpath);

  // if patch token binary, append the kernel name hash to the load module name
  switch (bkind){
  case gpu_binary_kind_intel_patch_token:
    strcat(load_module_name, ".");
    strncat(load_module_name, kernel_name_hash, CRYPTO_HASH_STRING_LENGTH);
  case gpu_binary_kind_elf:
    break;
  case gpu_binary_kind_unknown:
    EEMSG("FATAL: hpcrun failure: level 0 encountered unknown binary kind");
    auditor_exports->exit(-1);
  }

  // Step 4: insert the load module
  uint32_t module_id = gpu_binary_loadmap_insert(load_module_name, true /* mark_used */);

  return module_id;
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

  // Set up placeholder type
  gpu_op_placeholder_flags_t gpu_op_placeholder_flags = 0;
  gpu_placeholder_type_t gpu_placeholder_node;
  switch (command_node->type) {
    case LEVEL0_KERNEL: {
      gpu_op_placeholder_flags_set(&gpu_op_placeholder_flags, gpu_placeholder_type_kernel);
      gpu_op_placeholder_flags_set(&gpu_op_placeholder_flags, gpu_placeholder_type_trace);
      gpu_placeholder_node = gpu_placeholder_type_kernel;
      break;
    }
    case LEVEL0_MEMCPY: {
      ze_memory_type_t src_type = command_node->details.memcpy.src_type;
      ze_memory_type_t dst_type = command_node->details.memcpy.dst_type;

      // use src and dst types to distinguish copyin and copyout
      if (src_type == ZE_MEMORY_TYPE_DEVICE && dst_type != ZE_MEMORY_TYPE_DEVICE) {
        gpu_op_placeholder_flags_set(&gpu_op_placeholder_flags, gpu_placeholder_type_copyout);
        gpu_placeholder_node = gpu_placeholder_type_copyout;
      } else if (src_type != ZE_MEMORY_TYPE_DEVICE && dst_type == ZE_MEMORY_TYPE_DEVICE) {
        gpu_op_placeholder_flags_set(&gpu_op_placeholder_flags, gpu_placeholder_type_copyin);
        gpu_placeholder_node = gpu_placeholder_type_copyin;
      } else {
        gpu_op_placeholder_flags_set(&gpu_op_placeholder_flags, gpu_placeholder_type_copy);
        gpu_placeholder_node = gpu_placeholder_type_copy;
      }
      break;
    }
    default:
      // FIXME: need to set gpu_placeholder_node to none, but there is not a none value
      break;
  }

  uint64_t correlation_id = 0;
  if (level0_pcsampling_enabled()) {
    correlation_id = gpu_activity_channel_generate_correlation_id();
  } else {
    correlation_id = (uint64_t)(command_node->event);
  }

  command_node->correlation_id = correlation_id;
  
  cct_node_t *api_node =
    gpu_application_thread_correlation_callback(correlation_id);

  gpu_op_ccts_t gpu_op_ccts;
  hpcrun_safe_enter();
  gpu_op_ccts_insert(api_node, &gpu_op_ccts, gpu_op_placeholder_flags);

  if (command_node->type == LEVEL0_KERNEL) {
    ip_normalized_t kernel_ip;
    ze_kernel_handle_t kernel = command_node->details.kernel.kernel;
    size_t name_size = 0;
    zeKernelGetName(kernel, &name_size, NULL);
    char* kernel_name = malloc(name_size);
    zeKernelGetName(kernel, &name_size, kernel_name);
#ifdef ENABLE_GTPIN
    if (level0_gtpin_enabled()) {
      kernel_ip = gtpin_lookup_kernel_ip(kernel_name);
    } else
#endif  // ENABLE_GTPIN
    {
      if (level0_pcsampling_enabled()) {
        kernel_ip = level0_func_ip_resolve(kernel);
      } else {
        kernel_ip = gpu_kernel_table_get(kernel_name, LOGICAL_MANGLING_CPP);
      }
    }
    free(kernel_name);

    cct_node_t *kernel_ph = gpu_op_ccts_get(&gpu_op_ccts, gpu_placeholder_type_kernel);
    command_node->kernel =
    gpu_cct_insert_always(kernel_ph, kernel_ip);

    cct_node_t *trace_ph = gpu_op_ccts_get(&gpu_op_ccts, gpu_placeholder_type_trace);
    gpu_cct_insert(trace_ph, kernel_ip);
  }

  command_node->cct_node = gpu_op_ccts_get(&gpu_op_ccts, gpu_placeholder_node);

  hpcrun_safe_exit();

  // Generate host side operation timestamp
  command_node->submit_time = hpcrun_nanotime();

  if (level0_pcsampling_enabled() && command_node->type == LEVEL0_KERNEL) {
      gpu_activity_channel_t *channel = gpu_activity_channel_get_local();
      gpu_correlation_channel_send(1, correlation_id, channel);
      printf("level0_command_begin: correlation_id %lu\n", correlation_id, ", channel %p\n", channel);
      gpu_op_ccts_map_insert(correlation_id, (gpu_op_ccts_map_entry_value_t) {
        .gpu_op_ccts = gpu_op_ccts,
        .cpu_submit_time = command_node->submit_time
      });
  }

#ifdef ENABLE_GTPIN
  if (command_node->type == LEVEL0_KERNEL && level0_gtpin_enabled()) {
    // Callback to produce gtpin correlation
    gtpin_produce_runtime_callstack(&gpu_op_ccts);
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
  if (level0_pcsampling_enabled() && command_node->type == LEVEL0_KERNEL) {
    level0_pcsamples_sync(command_node, &gpu_activity_mtx, &cv, &data_processed);
  }
  
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

  // Send this activity record to the hpcrun monitoring thread
  gpu_operation_multiplexer_push(
    command_node->initiator_channel,
    command_node->pending_operations,
    ga
  );
}

void
level0_flush_and_wait
(
  void
)
{
  atomic_bool wait;
  atomic_store(&wait, true);
  gpu_activity_t gpu_activity;
  memset(&gpu_activity, 0, sizeof(gpu_activity_t));

  gpu_activity.kind = GPU_ACTIVITY_FLUSH;
  gpu_activity.details.flush.wait = &wait;
  gpu_operation_multiplexer_push(gpu_activity_channel_get_local(), NULL, &gpu_activity);

  // Wait until operations are drained
  // Operation channel is FIFO
  while (atomic_load(&wait)) {}
}

void
level0_wait_for_self_pending_operations
(
  void
)
{
  while (atomic_load(&level0_self_pending_operations) != 0);
}
