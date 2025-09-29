// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "level0-api.h"
#include "level0-pc-shim.h"
#include "level0-binary.h"
#include "level0-command-list-map.h"
#include "level0-command-list-context-map.h"
#include "level0-command-process.h"
#include "level0-command-queue-map.h"
#include "level0-event-map.h"
#include "level0-data-node.h"
#include "level0-debug.h"
#include "level0-fence-map.h"
#include "level0-id-map.h"
#include "level0-kernel-module-map.h"
#include "pcsampling/level0-correlation-device-map.h"
#include "pcsampling/level0-pc-manager.hpp"

#include "../../../../utilities/linuxtimer.h"

#include "../../../../libmonitor/monitor.h"
#include "../../../../main.h"
#include "../../../../memory/hpcrun-malloc.h"
#include "../../../gpu-monitoring-thread-api.h"
#include "../../../gpu-application-thread-api.h"
#include "../../../trace/gpu-trace-api.h"
#include "../../common/gpu-kernel-table.h"
#include "../../../../foil/level0.h"
#include "../../../../libmonitor/monitor.h"
#include "../../../../utilities/hpcrun-nanotime.h"

#ifdef ENABLE_GTPIN
#include "../gtpin/gtpin-instrumentation.h"
#endif



//******************************************************************************
// macros
//******************************************************************************

#define LATE_BEGIN 0

#define GPU_FLUSH_ALARM_ENABLED 1
#define GPU_FLUSH_ALARM_TEST_ENABLED 0
#include "../../common/gpu-flush-alarm.h"



//******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0
#include "../../../common/gpu-print.h"



//******************************************************************************
// local variables
//******************************************************************************

// Assume one driver and one device.
ze_driver_handle_t hDriver = NULL;
ze_device_handle_t hDevice = NULL;

uint64_t clock_offset_ns_from_level0 = 0;

static bool gtpin_instrumentation = false;
static bool level0_metrics_env = false;
static bool level0_pc_sampling_requested = false;  // Track if PC sampling was requested via event
static bool level0_pc_sampling_initialized = false; // Track if PC sampling library was initialized
static const struct hpcrun_foil_appdispatch_level0* saved_dispatch = NULL; // Save dispatch for deferred init

//******************************************************************************
// private operations
//******************************************************************************

static void
compute_device_time_offset
(
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0 *dispatch
)
{
  uint64_t hostTimestamp, deviceTimestamp;
  f_zeDeviceGetGlobalTimestamps(hDevice, &hostTimestamp, &deviceTimestamp, dispatch);

  // using the host timestamp from level0 didn't produce a useful result.
  // try setting hostTimestamp from hpcrun's time on host instead
  hostTimestamp = hpcrun_nanotime();

  clock_offset_ns_from_level0 = hostTimestamp - deviceTimestamp;

  PRINT("level0: host time %ld device time %ld offset %ld\n",
    hostTimestamp, deviceTimestamp, clock_offset_ns_from_level0);
}


static void
level0_check_result
(
  ze_result_t result,
  int lineNo
)
{
  if (result == ZE_RESULT_SUCCESS) return;

  EEMSG("hpcrun: Level Zero API failed: %s",
        ze_result_to_string(result));

  exit(1);
}


static void
get_gpu_driver_and_device
(
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (hDevice != NULL) return;
  uint32_t driverCount = 0;
  uint32_t i, d;
  f_zeDriverGet(&driverCount, NULL, dispatch);

  ze_driver_handle_t* allDrivers = (ze_driver_handle_t*)hpcrun_malloc_safe(driverCount * sizeof(ze_driver_handle_t));
  f_zeDriverGet(&driverCount, allDrivers, dispatch);
  PRINT("Get %d driver handles\n", driverCount);

  // Find a driver instance with a GPU device
  for(i = 0; i < driverCount; ++i) {
    uint32_t deviceCount = 0;
    f_zeDeviceGet(allDrivers[i], &deviceCount, NULL, dispatch);
    PRINT("\tGet %d device handles\n", deviceCount);

    ze_device_handle_t* allDevices = (ze_device_handle_t*)hpcrun_malloc_safe(deviceCount * sizeof(ze_device_handle_t));
    f_zeDeviceGet(allDrivers[i], &deviceCount, allDevices, dispatch);

    for(d = 0; d < deviceCount; ++d) {
      ze_device_properties_t device_properties;
      device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
      device_properties.pNext = NULL;
      f_zeDeviceGetProperties(allDevices[d], &device_properties, dispatch);
      if(ZE_DEVICE_TYPE_GPU == device_properties.type) {
        hDriver = allDrivers[i];
        hDevice = allDevices[d];
        break;
      }
    }
    if(NULL != hDriver) {
      break;
    }
  }

  if (NULL == hDevice) {
    EEMSG("hpcrun: Level Zero failed: no GPU device found");
    exit(1);
  }

  compute_device_time_offset(hDevice, dispatch);
}


static void
level0_create_new_event
(
  ze_context_handle_t hContext,
  ze_event_handle_t* event_ptr,
  ze_event_pool_handle_t* event_pool_ptr,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{

  ze_event_pool_desc_t event_pool_desc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
    NULL,
    ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, // all events in pool are kernel timestamps
    1 // count
  };
  f_zeEventPoolCreate(hContext, &event_pool_desc, 1, &hDevice, event_pool_ptr, dispatch);

  ze_event_desc_t event_desc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC,
    NULL,
    0, // index
    0, // no memory/cache coherency required on signal
    0  // no memory/cache coherency required on wait
  };
  f_zeEventCreate(*event_pool_ptr, &event_desc, event_ptr, dispatch);
}


static void
level0_attribute_event
(
  ze_event_handle_t event,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("level0_attribute_event for event %p\n", event);
  level0_data_node_t* data = level0_event_map_lookup(event);
  if (data == NULL) return;

  // Get ready to query time stamps
  ze_device_properties_t props;
  props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES ;
  props.pNext = NULL;
  f_zeDeviceGetProperties(hDevice, &props, dispatch);

  ze_result_t status = f_zeEventQueryStatus(event, dispatch);
  level0_check_result(status, __LINE__);

  // Query start and end time stamp for the event
  ze_kernel_timestamp_result_t timestamp;
  f_zeEventQueryKernelTimestamp(event, &timestamp, dispatch);
  uint64_t start = timestamp.global.kernelStart * props.timerResolution;
  uint64_t end = timestamp.global.kernelEnd * props.timerResolution;

  // Attribute this event
  level0_command_end(data, start, end);

  // We need to release the event and event_pool to level 0
  // if they are created by us.
  if (data->event_pool != NULL) {
    f_zeEventDestroy(event, dispatch);
    f_zeEventPoolDestroy(data->event_pool, dispatch);
  }

  // Free data structure for this event
  level0_event_map_delete(event);
}


static void
level0_get_memory_types
(
  ze_context_handle_t hContext,
  const void* src_ptr,
  const void* dest_ptr,
  ze_memory_type_t *src_type_ptr,
  ze_memory_type_t *dst_type_ptr,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Get source and destination type.
  // Level 0 does not track memory allocated through system allocator such as malloc.
  // In such case, zeDriverGetMemAllocProperties will return failure.
  // So, we default the memory type to be HOST.
  ze_memory_allocation_properties_t property;
  property.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  property.pNext = NULL;
  if (f_zeMemGetAllocProperties(hContext, src_ptr, &property, NULL, dispatch) == ZE_RESULT_SUCCESS) {
    *src_type_ptr = property.type;
  }
  if (f_zeMemGetAllocProperties(hContext, dest_ptr, &property, NULL, dispatch) == ZE_RESULT_SUCCESS) {
    *dst_type_ptr = property.type;
  }
}


static void
level0_event_pool_create_entry
(
  const ze_event_pool_desc_t* desc,
  ze_event_pool_desc_t* pool_desc
)
{
  if (desc == NULL) {
    // Based on Level 0 header file,
    // zeEventPoolCreate will return ZE_RESULT_ERROR_INVALID_NULL_POINTER for this caes.
    // Therefore, we do nothing in this case.
    return;
  }

  // Here we need to allocate a new event pool descriptor
  // as we cannot directly change the passed in object (declared ad const)
  // This leads to one description per event pool creation.
  pool_desc->flags = desc->flags;
  pool_desc->count = desc->count;
  pool_desc->stype = desc->stype;
  pool_desc->pNext = desc->pNext;

  // We attach the time stamp flag to the event pool,
  // so that we can query time stamps for events in this pool.
  int flags = pool_desc->flags | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  pool_desc->flags = (ze_event_pool_flag_t)(flags);
}


static ze_event_handle_t
level0_command_list_append_launch_kernel_entry
(
  ze_kernel_handle_t kernel,
  ze_command_list_handle_t command_list,
  ze_event_handle_t event,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_event_pool_handle_t event_pool = NULL;

  if (event == NULL) {
    ze_context_handle_t hContext = level0_commandlist_context_map_lookup(command_list);
    // If the kernel is launched without an event,
    // we create a new event for collecting time stamps
    level0_create_new_event(hContext, &event, &event_pool, dispatch);
  }

  PRINT("level0_command_list_append_launch_kernel_entry: kernel handle %p, command list handle %p, event handle %p, event pool handle %p\n",
    (void*)kernel, (void*)command_list, (void*)event, (void*)event_pool);

  // Lookup the command list and append the kernel launch to the command list
  level0_data_node_t ** command_list_data_head = level0_commandlist_map_lookup(command_list);
  int32_t device_id = hpcrun_level0_cmdlist_device_lookup(command_list);
  if (command_list_data_head != NULL) {
    level0_data_node_t * data_for_kernel = level0_commandlist_append_kernel(command_list_data_head, kernel, event, event_pool, dispatch);
    data_for_kernel->device_id = device_id;
    // Associate the data entry with the event
    level0_event_map_insert(event, data_for_kernel);
  } else {
    // Cannot find command list.
    // This means we are dealing with an immediate command list
    level0_data_node_t * data_for_kernel = level0_commandlist_alloc_kernel(kernel, event, event_pool, dispatch);
    data_for_kernel->device_id = device_id;
    // Associate the data entry with the event
    level0_event_map_insert(event, data_for_kernel);
#if LATE_BEGIN == 0
    // For immediate command list, the kernel is dispatched to GPU at this point.
    // So, we attribute GPU metrics to the current CPU calling context.
    level0_command_begin(data_for_kernel);
#endif
  }
  return event;
}


static ze_event_handle_t
level0_command_list_append_launch_memcpy_entry
(
  ze_command_list_handle_t command_list,
  ze_event_handle_t event,
  size_t mem_copy_size,
  const void* dest_ptr,
  const void* src_ptr,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_event_pool_handle_t event_pool = NULL;
  ze_context_handle_t hContext = level0_commandlist_context_map_lookup(command_list);
  if (event == NULL) {
    // If the memcpy is launched without an event,
    // we create a new event for collecting time stamps
    level0_create_new_event(hContext, &event, &event_pool, dispatch);
  }

  ze_memory_type_t src_type = ZE_MEMORY_TYPE_HOST;
  ze_memory_type_t dst_type = ZE_MEMORY_TYPE_HOST;
  level0_get_memory_types(hContext, src_ptr, dest_ptr, &src_type, &dst_type, dispatch);

  PRINT("level0_command_list_append_launch_memcpy_entry: src_type %d, dst_type %d, size %lu, command list %p, event handle %p, event pool handle %p\n",
    src_type, dst_type, mem_copy_size, (void*)command_list, (void*)event, (void*)event_pool);

  // Lookup the command list and append the mempcy to the command list
  level0_data_node_t ** command_list_data_head = level0_commandlist_map_lookup(command_list);
  int32_t device_id = hpcrun_level0_cmdlist_device_lookup(command_list);
  if (command_list_data_head != NULL) {
    level0_data_node_t * data_for_memcpy = level0_commandlist_append_memcpy(command_list_data_head, src_type, dst_type, mem_copy_size, event, event_pool, dispatch);
    data_for_memcpy->device_id = device_id;
    // Associate the data entry with the event
    level0_event_map_insert(event, data_for_memcpy);
  } else {
    // Cannot find command list.
    // This means we are dealing with an immediate command list
    level0_data_node_t * data_for_memcpy = level0_commandlist_alloc_memcpy(src_type, dst_type, mem_copy_size, event, event_pool, dispatch);
    data_for_memcpy->device_id = device_id;
    // Associate the data entry with the event
    level0_event_map_insert(event, data_for_memcpy);
#if LATE_BEGIN == 0
    // For immediate command list, the mempcy is dispatched to GPU at this point.
    // So, we attribute GPU metrics to the current CPU calling context.
    level0_command_begin(data_for_memcpy);
#endif
  }
  return event;
}


static void
level0_command_list_create_exit
(
  ze_command_list_handle_t handle,
  ze_context_handle_t hContext,
  ze_device_handle_t hDevice,
  int isImmediateList
)
{
  PRINT("level0_command_list_create_exit: command list %p, context handle %p, imm list %d\n",
    (void*)handle, (void*)hContext, isImmediateList);
  // Record the creation of a command list
  // command list map: command list handle -> a list of kernel launches and memcpy
  if (!isImmediateList) {
    level0_commandlist_map_insert(handle);
  }
  // command list context map: command list handle -> context handle
  level0_commandlist_context_map_insert(handle, hContext);

  hpcrun_level0_cmdlist_device_register_with_device(handle, hDevice);
}


static void
level0_command_list_destroy_entry
(
  ze_command_list_handle_t handle,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  level0_data_node_t ** command_list_head = level0_commandlist_map_lookup(handle);
  level0_commandlist_context_map_delete(handle);

  // If this happens, it is an immedicate list
  if (command_list_head == NULL) {
    return;
  }

  level0_data_node_t * command_node = *command_list_head;
  for (; command_node != NULL; command_node = command_node->next) {
    level0_attribute_event(command_node->event, dispatch);
  }

  // Record the deletion of a command list
  level0_commandlist_map_delete(handle);
}


static void
level0_command_list_reset_entry
(
  ze_command_list_handle_t handle,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  level0_data_node_t ** command_list_head = level0_commandlist_map_lookup(handle);

  // If this happens, it is an immedicate list
  if (command_list_head == NULL) {
    return;
  }

  level0_data_node_t * command_node = *command_list_head;
  for (; command_node != NULL; command_node = command_node->next) {
    level0_attribute_event(command_node->event, dispatch);
  }

  // Reset the command list data to empty
  level0_data_list_free(*command_list_head);
  *command_list_head = NULL;
}


static void
level0_command_queue_execute_command_list_entry
(
  uint32_t numCommandLists,                       ///< [in] number of command lists to execute
  ze_command_list_handle_t* phCommandLists       ///< [in][range(0, numCommandLists)] list of handles of the command lists
)
{
  // We associate GPU metrics for GPU activitities in non-immediate command list
  // to the CPU call contexts where the command list is executed, not where
  // the GPU activity is appended.
  uint32_t i;
  for (i = 0; i < numCommandLists; ++i) {
    ze_command_list_handle_t command_list_handle = phCommandLists[i];
    PRINT("level0_command_queue_execute_command_list_entry: command list %p\n", (void*)command_list_handle);
    level0_data_node_t ** command_list_head = level0_commandlist_map_lookup(command_list_handle);
    level0_data_node_t * command_node = *command_list_head;
    for (; command_node != NULL; command_node = command_node->next) {
      level0_command_begin(command_node);
    }
  }
}


static void
level0_process_immediate_command_list
(
  ze_event_handle_t event,
  ze_command_list_handle_t command_list,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  level0_data_node_t ** command_list_data_head = level0_commandlist_map_lookup(command_list);
  if (command_list_data_head == NULL) {
    // This is a GPU activity to an immediate command list
    level0_data_node_t* data_for_act = level0_event_map_lookup(event);

#if LATE_BEGIN != 0
    // For immediate command list, the kernel is dispatched to GPU at this point.
    // So, we attribute GPU metrics to the current CPU calling context.
    level0_command_begin(data_for_act);
#endif

    level0_attribute_event(event, dispatch);

    // For command in immediate command list,
    // the ownership of data node belongs to the user, not the command list
    level0_data_node_return_free_list(data_for_act);
  }
}


static void
level0_attribute_fence
(
  ze_fence_handle_t hFence,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (hFence == NULL) return;

  level0_fence_data_t * data = level0_fence_map_lookup(hFence);
  if (data == NULL) return;

  for (int i = 0; i < data->num; ++i) {
    level0_command_list_reset_entry(data->list[i], dispatch);
  }

  level0_fence_map_delete(hFence);
}


static void
level0_attribute_command_queue
(
  ze_command_queue_handle_t command_queue,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  if (command_queue == NULL) return;

  level0_command_queue_data_t * data = level0_command_queue_map_lookup(command_queue);
  if (data == NULL) return;

  for (int i = 0; i < data->num; ++i) {
    level0_command_list_reset_entry(data->list[i], dispatch);
  }

  level0_command_queue_map_delete(command_queue);
}


//******************************************************************************
// interface operations
//******************************************************************************

ze_result_t
hpcrun_zeInit
(
  ze_init_flag_t flag,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeInit: enter\n");

  // programs can invoke zeInit in a static constructor before the measurement
  // subsystem has been initialized. force hpcrun initialization here if it
  // hasn't already been initialized
  monitor_initialize(); // early init necessary to set up libunwind
  hpcrun_prepare_measurement_subsystem(false); // late init for level0

  // ignore any threads created by Level Zero's zeInit
  monitor_disable_new_threads();

  // Entry action
  // Execute the real level0 API
  ze_result_t ret = f_zeInit(flag, dispatch);

  // resume tracking thread creation
  monitor_enable_new_threads();

  level0_check_result(ret, __LINE__);

  // Exit action
  get_gpu_driver_and_device(dispatch);

  // Save dispatch pointer for potential later use
  saved_dispatch = dispatch;

  // Initialize PC sampling if it was requested and not yet initialized
  // This handles the case where zeInit is called after level0_init (normal case)
  if (level0_pc_sampling_requested && !level0_pc_sampling_initialized) {
    char error_buffer[256] = {0};  // Initialize to avoid reading garbage
    level0_pc_result_t result = level0_pc_init(dispatch, error_buffer, sizeof(error_buffer));
    if (result == LEVEL0_PC_SUCCESS) {
      level0_pc_sampling_initialized = true;
      TMSG(LEVEL0, "PC sampling initialized successfully");
    } else {
      // Log the failure but don't set initialized flag, allowing retry
      EMSG("Failed to initialize Intel Level Zero PC sampling: %s (error code: %d)",
           error_buffer[0] ? error_buffer : "unknown error", result);
    }
  }

  PRINT("hpcrun_zeInit: exit\n");

  return ret;
}

ze_result_t
hpcrun_zeCommandListAppendLaunchKernel
(
  ze_command_list_handle_t hCommandList,          ///< [in] handle of the command list
  ze_kernel_handle_t hKernel,                     ///< [in] handle of the kernel object
  const ze_group_count_t* pLaunchFuncArgs,        ///< [in] thread group launch arguments
  ze_event_handle_t hSignalEvent,                 ///< [in][optional] handle of the event to signal on completion
  uint32_t numWaitEvents,                         ///< [in][optional] number of events to wait on before launching; must be 0
                                                  ///< if `nullptr == phWaitEvents`
  ze_event_handle_t* phWaitEvents,                ///< [in][optional][range(0, numWaitEvents)] handle of the events to wait
                                                  ///< on before launching
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListAppendLaunchKernel enter: command list %p\n", hCommandList);

  // Entry action:
  // We need to create a new event for querying time stamps
  // if the user appends the kernel with an empty event parameter
  ze_event_handle_t new_event_handle = level0_command_list_append_launch_kernel_entry(
    hKernel, hCommandList, hSignalEvent, dispatch);

  // Execute the real level0 API
  ze_result_t ret = f_zeCommandListAppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs,
    new_event_handle, numWaitEvents, phWaitEvents, dispatch);

#if 0
  if (level0_metrics_requested()) {
    f_zeEventHostSynchronize(new_event_handle, UINT64_MAX - 1, dispatch);
  }
#endif

  // Exit action
  level0_process_immediate_command_list(new_event_handle, hCommandList, dispatch);

  PRINT("hpcrun_zeCommandListAppendLaunchKernel exit\n");

  return ret;
}

ze_result_t
hpcrun_zeCommandListAppendMemoryCopy
(
  ze_command_list_handle_t hCommandList,          ///< [in] handle of command list
  void* dstptr,                                   ///< [in] pointer to destination memory to copy to
  const void* srcptr,                             ///< [in] pointer to source memory to copy from
  size_t size,                                    ///< [in] size in bytes to copy
  ze_event_handle_t hSignalEvent,                 ///< [in][optional] handle of the event to signal on completion
  uint32_t numWaitEvents,                         ///< [in][optional] number of events to wait on before launching; must be 0
                                                  ///< if `nullptr == phWaitEvents`
  ze_event_handle_t* phWaitEvents,                ///< [in][optional][range(0, numWaitEvents)] handle of the events to wait
                                                  ///< on before launching
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListAppendMemoryCopy enter: command list %p\n", hCommandList);

  // Entry action:
  // We need to create a new event for querying time stamps
  // if the user appends the kernel with an empty event parameter
  ze_event_handle_t new_event_handle =
  level0_command_list_append_launch_memcpy_entry(
      hCommandList, hSignalEvent, size, dstptr, srcptr, dispatch);
  // Execute the real level0 API
  ze_result_t ret = f_zeCommandListAppendMemoryCopy(hCommandList, dstptr, srcptr, size, new_event_handle, numWaitEvents, phWaitEvents, dispatch);

  // Exit action
  level0_process_immediate_command_list(new_event_handle, hCommandList, dispatch);

  PRINT("hpcrun_zeCommandListAppendMemoryCopy exit\n");

  return ret;
}


ze_result_t
hpcrun_zeCommandListCreate
(
  ze_context_handle_t hContext,                   ///< [in] handle of the context object
  ze_device_handle_t hDevice,                     ///< [in] handle of the device object
  const ze_command_list_desc_t* desc,             ///< [in] pointer to command list descriptor
  ze_command_list_handle_t* phCommandList,        ///< [out] pointer to handle of command list object created
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListCreate enter\n");

  // Entry action
  // Execute the real level0 API
  ze_result_t ret = f_zeCommandListCreate(hContext, hDevice, desc, phCommandList, dispatch);

  // Exit action
  level0_command_list_create_exit(*phCommandList, hContext, hDevice, 0);

  PRINT("hpcrun_zeCommandListCreate exit\n");

  return ret;
}


ze_result_t
hpcrun_zeCommandListCreateImmediate
(
  ze_context_handle_t hContext,                   ///< [in] handle of the context object
  ze_device_handle_t hDevice,                     ///< [in] handle of the device object
  const ze_command_queue_desc_t* altdesc,         ///< [in] pointer to command queue descriptor
  ze_command_list_handle_t* phCommandList,        ///< [out] pointer to handle of command list object created
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListCreateImmediate enter\n");

  // Entry action
  // Execute the real level0 API

  // hpctoolkit doesn't yet properly synchronize asynchronous command lists
  // FIXME: force the execution to be synchronous
  ze_command_queue_desc_t altdesc_new = *altdesc;
  altdesc_new.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  altdesc = &altdesc_new;

  ze_result_t ret = f_zeCommandListCreateImmediate(hContext, hDevice, altdesc, phCommandList, dispatch);

  // Exit action
  level0_command_list_create_exit(*phCommandList, hContext, hDevice, 1);

  PRINT("hpcrun_zeCommandListCreateImmediate exit\n");

  return ret;
}


ze_result_t
hpcrun_zeCommandListDestroy
(
  ze_command_list_handle_t hCommandList,          ///< [in][release] handle of command list object to destroy
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListDestroy enter: command list %p\n", hCommandList);

  // Entry action
  level0_command_list_destroy_entry(hCommandList, dispatch);
  // Execute the real level0 API
  ze_result_t ret = f_zeCommandListDestroy(hCommandList, dispatch);
  // Exit action

  PRINT("hpcrun_zeCommandListDestroy exit\n");

  return ret;
}


ze_result_t
hpcrun_zeCommandListReset
(
  ze_command_list_handle_t hCommandList,          ///< [in] handle of command list object to reset
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandListReset enter: command list %p\n", hCommandList);

  // Entry action
  level0_command_list_reset_entry(hCommandList, dispatch);
  // Execute the real level0 API
  ze_result_t ret = f_zeCommandListReset(hCommandList, dispatch);
  // Exit action

  PRINT("hpcrun_zeCommandListReset exitp\n");

  return ret;
}


ze_result_t
hpcrun_zeCommandQueueExecuteCommandLists
(
  ze_command_queue_handle_t hCommandQueue,        ///< [in] handle of the command queue
  uint32_t numCommandLists,                       ///< [in] number of command lists to execute
  ze_command_list_handle_t* phCommandLists,       ///< [in][range(0, numCommandLists)] list of handles of the command lists
                                                  ///< to execute
  ze_fence_handle_t hFence,                       ///< [in][optional] handle of the fence to signal on completion
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandQueueExecuteCommandLists enter: command queue %p, fence %p\n",
    hCommandQueue, hFence);

  // Entry action
  level0_command_queue_execute_command_list_entry(numCommandLists, phCommandLists);
  level0_fence_map_insert(hFence, numCommandLists, phCommandLists);
  level0_command_queue_map_insert(hCommandQueue, numCommandLists, phCommandLists);
  // Execute the real level0 API
  ze_result_t ret = f_zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists, phCommandLists, hFence, dispatch);
  // Exit action

  PRINT("hpcrun_zeCommandQueueExecuteCommandLists exit\n");

  return ret;
}


ze_result_t
hpcrun_zeEventPoolCreate
(
  ze_context_handle_t hContext,                   ///< [in] handle of the context object
  const ze_event_pool_desc_t* desc,               ///< [in] pointer to event pool descriptor
  uint32_t numDevices,                            ///< [in][optional] number of device handles; must be 0 if `nullptr ==
                                                  ///< phDevices`
  ze_device_handle_t* phDevices,                  ///< [in][optional][range(0, numDevices)] array of device handles which
                                                  ///< have visibility to the event pool.
                                                  ///< if nullptr, then event pool is visible to all devices supported by the
                                                  ///< driver instance.
  ze_event_pool_handle_t* phEventPool,            ///< [out] pointer handle of event pool object created
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeEventPoolCreate enter\n");

  // Entry action
  ze_event_pool_desc_t pool_desc;
  level0_event_pool_create_entry(desc, &pool_desc);
  // Execute the real level0 API
  ze_result_t ret;
  if (desc == NULL) {
    ret = f_zeEventPoolCreate(hContext, NULL, numDevices, phDevices, phEventPool, dispatch);
  } else {
    ret = f_zeEventPoolCreate(hContext, &pool_desc, numDevices, phDevices, phEventPool, dispatch);
  }
  // Exit action

  PRINT("hpcrun_zeEventPoolCreate exit\n");

  return ret;
}


ze_result_t
hpcrun_zeEventDestroy
(
  ze_event_handle_t hEvent,                       ///< [in][release] handle of event object to destroy
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeEventDestroy enter\n");

  // Entry action
  level0_attribute_event(hEvent, dispatch);
  // Execute the real level0 API
  ze_result_t ret = f_zeEventDestroy(hEvent, dispatch);
  // Exit action

  PRINT("hpcrun_zeEventDestroy exit\n");

  return ret;
}


ze_result_t
hpcrun_zeEventHostReset
(
  ze_event_handle_t hEvent,                       ///< [in] handle of the event
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeEventHostReset enter\n");

  // Entry action
  level0_attribute_event(hEvent, dispatch);
  // Execute the real level0 API
  ze_result_t ret = f_zeEventHostReset(hEvent, dispatch);

  // Exit action

  PRINT("hpcrun_zeEventHostReset exit\n");

  return ret;
}


ze_result_t
hpcrun_zeModuleCreate
(
  ze_context_handle_t hContext,                // [in] handle of the context object
  ze_device_handle_t hDevice,                  // [in] handle of the device
  const ze_module_desc_t *desc,                // [in] pointer to module descriptor
  ze_module_handle_t *phModule,                // [out] pointer to handle of module object created
  ze_module_build_log_handle_t *phBuildLog,    // [out][optional] pointer to handle of module’s build log.
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  char compile_flags[128] = {0};
  ze_module_desc_t new_desc = *desc;
  if (new_desc.format == ZE_MODULE_FORMAT_IL_SPIRV) {
    // if the module is created through SPIRV IR,
    // it will go through JIT compilation, and we can append
    // the -g flag to add debug information
    if (desc->pBuildFlags) strcpy(compile_flags, desc->pBuildFlags);
    strcat(compile_flags, " -g");
    new_desc.pBuildFlags = compile_flags;
  }

  ze_result_t ret = f_zeModuleCreate(hContext, hDevice, &new_desc, phModule, phBuildLog, dispatch);
  PRINT("hpcrun_zeModuleCreate: module handle %p\n", *phModule);

  // Exit action
  level0_binary_process(*phModule, dispatch);

  return ret;
}

ze_result_t
hpcrun_zeModuleDestroy
(
  ze_module_handle_t hModule,      // [in][release] handle of the module
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Entry action
  level0_module_handle_map_delete(hModule);

  // Hash the module handle to get a unique id
  char zebin_id[CRYPTO_HASH_STRING_LENGTH];
  crypto_compute_hash_string(&hModule, sizeof(hModule), zebin_id, CRYPTO_HASH_STRING_LENGTH);
  uint32_t zebin_id_uint32;
  sscanf(zebin_id, "%8x", &zebin_id_uint32);
  zebin_id_map_delete(zebin_id_uint32);

  ze_result_t ret = f_zeModuleDestroy(hModule, dispatch);

  return ret;
}

ze_result_t
hpcrun_zeKernelCreate
(
  ze_module_handle_t hModule,          // [in] handle of the module
  const ze_kernel_desc_t *desc,        // [in] pointer to kernel descriptor
  ze_kernel_handle_t *phKernel,        // [out] handle of the Function object
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  ze_result_t ret = f_zeKernelCreate(hModule, desc, phKernel, dispatch);

  PRINT("hpcrun_zeKernelCreate: module handle %p, kernel handle %p\n",hModule, *phKernel);
  
  // Exit action - save kernel-module mapping for PC sampling
  level0_kernel_module_map_insert(*phKernel, hModule);

  return ret;
}

ze_result_t
hpcrun_zeKernelDestroy
(
  ze_kernel_handle_t hKernel,     // [in][release] handle of the kernel object
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Entry action - remove kernel-module mapping
  level0_kernel_module_map_delete(hKernel);
  
  ze_result_t ret = f_zeKernelDestroy(hKernel, dispatch);

  return ret;
}

ze_result_t
hpcrun_zeFenceDestroy
(
  ze_fence_handle_t hFence,       // [in][release] handle of fence object to destroy
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeFenceDestroy enter: fence %p\n", hFence);

  level0_attribute_fence(hFence, dispatch);
  ze_result_t ret = f_zeFenceDestroy(hFence, dispatch);

  PRINT("hpcrun_zeFenceDestroy exit\n");

  return ret;
}


ze_result_t
hpcrun_zeFenceReset
(
  ze_fence_handle_t hFence,      //  [in] handle of the fence
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeFenceReset enter: fence %p\n", hFence);

  level0_attribute_fence(hFence, dispatch);
  ze_result_t ret = f_zeFenceReset(hFence, dispatch);

  PRINT("hpcrun_zeFenceReset exit\n");

  return ret;
}

ze_result_t
hpcrun_zeCommandQueueSynchronize
(
  ze_command_queue_handle_t hCommandQueue,   // [in] handle of the command queue
  uint64_t timeout,                          // [in] if non-zero, then indicates the maximum time (in nanoseconds) to yield before returning
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  PRINT("hpcrun_zeCommandQueueSynchronize enter: command queue %p\n", hCommandQueue);

  ze_result_t ret = f_zeCommandQueueSynchronize(hCommandQueue, timeout, dispatch);
  level0_attribute_command_queue(hCommandQueue, dispatch);

  PRINT("hpcrun_zeCommandQueueSynchronize exit\n");

  return ret;
}

void
level0_init
(
 gpu_instrumentation_t *inst_options
)
{
  if (gpu_instrumentation_enabled(inst_options)) {
#ifdef ENABLE_GTPIN
    gtpin_instrumentation = true;
    gtpin_instrumentation_options(inst_options);
#endif
  }

  // Update PC sampling request state based on current event configuration
  // This ensures the flag is properly reset when PC sampling is not requested
  level0_pc_sampling_requested = (inst_options && inst_options->pc_sampling);

  // Check if PC sampling was requested through the event (e.g., gpu=level0,pc)
  if (level0_pc_sampling_requested) {
    level0_metrics_env = true;
    // Set the environment variable for Level Zero metrics if not already set
    setenv("ZET_ENABLE_METRICS", "1", 0);  // 0 means don't overwrite if exists

    // If zeInit was already called (saved_dispatch != NULL), initialize PC sampling now
    // This handles the case where zeInit is called before level0_init (static constructor case)
    if (saved_dispatch && !level0_pc_sampling_initialized) {
      char error_buffer[256] = {0};  // Initialize to avoid reading garbage
      level0_pc_result_t result = level0_pc_init(saved_dispatch, error_buffer, sizeof(error_buffer));
      if (result == LEVEL0_PC_SUCCESS) {
        level0_pc_sampling_initialized = true;
        TMSG(LEVEL0, "PC sampling initialized successfully (deferred)");
      } else {
        // Log the failure but don't set initialized flag, allowing retry
        EMSG("Failed to initialize Intel Level Zero PC sampling (deferred): %s (error code: %d)",
             error_buffer[0] ? error_buffer : "unknown error", result);
      }
    }
  } else {
    // PC sampling not requested for this run
    level0_metrics_env = false;
  }

  if (!gtpin_instrumentation) {
    gpu_kernel_table_init();
  }
}

void
level0_fini
(
 void* args,
 int how
)
{
  if (!GPU_FLUSH_ALARM_FIRED()) {
    GPU_FLUSH_ALARM_SET("hpcrun: warning: some Level 0 events not marked"
                        " complete; some GPU event data may be lost.");

    // Only shutdown PC sampling if it was initialized
    if (level0_pc_sampling_initialized) {
      level0_pc_shutdown();
      level0_pc_sampling_initialized = false;
    }

    GPU_FLUSH_ALARM_TEST();
    GPU_FLUSH_ALARM_CLEAR();
  }

  // even if this is not normal exit, gpu-trace-fini will behave as if it is a normal exit
  gpu_trace_fini(NULL, MONITOR_EXIT_NORMAL);
}

void
level0_flush
(
 void *args,
 int how
)
{
  level0_flush_and_wait();

  // Wait until my activities are drained
  if (how == MONITOR_EXIT_NORMAL) level0_wait_for_self_pending_operations();

  // Now I can attribute activities
  gpu_application_thread_process_activities();
}

bool
level0_gtpin_enabled
(
  void
)
{
  return gtpin_instrumentation;
}

// adjust device timestamp to be consistent with host realtime
uint64_t
level0_timestamp_to_realtime
(
  uint64_t host_submit_time,
  uint64_t device_time
)
{
#if 0
  // adjust device timestamp with offset between host and device
  // that was computed when device was configured for use.
  uint64_t result = device_time + clock_offset_ns_from_level0;
#else
  // approximately compute the offset between device and host by assuming
  // that the first GPU operation reported executes on the host at the
  // time it was submitted.
  static volatile _Atomic uint64_t clock_offset_ns_from_submit = 0;
  if (clock_offset_ns_from_submit == 0) {
     uint64_t zero = 0;
     uint64_t offset = host_submit_time - device_time;
     atomic_compare_exchange_strong(&clock_offset_ns_from_submit, &zero, offset);
  }
#endif

  uint64_t result = device_time + clock_offset_ns_from_submit;

  PRINT("level0_timestamp_to_realtime(%ld) --> %ld\n", device_time, result);

  return result;
}

bool
level0_metrics_requested
(
  void
)
{
  return level0_metrics_env;
}
