// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// include files
//*****************************************************************************

#define _GNU_SOURCE

#include "level0-api.h"
#include "level0-device-properties.h"

#include "../../../../../common/lean/splay-uint64.h"
#include "../../../../../common/lean/spinlock.h"
#include "../../../../memory/hpcrun-malloc.h"
#include "../../../../utilities/hpcrun-nanotime.h"
#include "../../../gpu-splay-allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



//*****************************************************************************
// debugging
//*****************************************************************************

#define DEBUG 0

#include "../../../common/gpu-print.h"



//*****************************************************************************
// macros
//*****************************************************************************

#define GPU_TIMESTAMP_VALIDITY_THRESHOLD_NS 100000000   // 100ms
#define NS_PER_SECOND 1000000000ULL


//*****************************************************************************
// splay tree configuration macros
//*****************************************************************************

#define st_insert                               \
  typed_splay_insert(device_properties)

#define st_lookup                               \
  typed_splay_lookup(device_properties)

#define st_delete                               \
  typed_splay_delete(device_properties)

#define st_forall                               \
  typed_splay_forall(device_properties)

#define st_count                                \
  typed_splay_count(device_properties)

#define st_alloc(free_list)                     \
  typed_splay_alloc(free_list, level0_device_properties_map_entry_t)

#define st_free(free_list, node)                \
  typed_splay_free(free_list, node)



//*****************************************************************************
// type declarations
//*****************************************************************************

#undef typed_splay_node
#define typed_splay_node(device_properties) level0_device_properties_map_entry_t

typedef struct typed_splay_node(device_properties) {
  struct typed_splay_node(device_properties) *left;
  struct typed_splay_node(device_properties) *right;
  uintptr_t hDevice; // key
  level0_device_properties_t l0props; // value: can also be any pointer type
} typed_splay_node(device_properties);



//*****************************************************************************
// local data
//*****************************************************************************

static level0_device_properties_map_entry_t *map_root = NULL;
static level0_device_properties_map_entry_t *free_list = NULL;
static spinlock_t map_lock = SPINLOCK_UNLOCKED;



//*****************************************************************************
// private operations
//*****************************************************************************

typed_splay_impl(device_properties)


static level0_device_properties_map_entry_t *
level0_device_properties_map_entry_alloc(level0_device_properties_map_entry_t **free_list_ptr)
{
  return st_alloc(free_list_ptr);
}


static level0_device_properties_map_entry_t *
level0_device_properties_map_lookup
(
 level0_device_properties_map_entry_t** map_root_ptr,
 uintptr_t key
)
{
  level0_device_properties_map_entry_t *result = st_lookup(map_root_ptr, key);
  return result;
}

static void
level0_device_properties_map_insert
(
 level0_device_properties_map_entry_t** map_root_ptr,
 level0_device_properties_map_entry_t* new_entry
)
{
  st_insert(map_root_ptr, new_entry);
}


__attribute__((unused))
static void
level0_device_properties_map_delete
(
 level0_device_properties_map_entry_t** map_root_ptr,
 level0_device_properties_map_entry_t** free_list_ptr,
 uint64_t key
)
{
  level0_device_properties_map_entry_t *node = st_delete(map_root_ptr, key);
  st_free(free_list_ptr, node);
}

static level0_device_properties_map_entry_t*
level0_device_properties_map_entry_new
(
 level0_device_properties_map_entry_t **free_list,
 uintptr_t key
)
{
  level0_device_properties_map_entry_t *e = level0_device_properties_map_entry_alloc(free_list);

  memset(e, 0, sizeof(level0_device_properties_map_entry_t));
  e->hDevice = key;

  return e;
}


static void
level0_update_timestamps
(
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0 *dispatch,
  level0_device_properties_t *properties
)
{
  uint64_t *host_time = &properties->recent_synchronized_timestamps.host;
  uint64_t *device_time =&properties->recent_synchronized_timestamps.device;
  ze_result_t status = f_zeDeviceGetGlobalTimestamps(hDevice, host_time,
    device_time, dispatch);

  // read host CLOCK_REALTIME immediately after obtaining a device timestamp
  uint64_t real = hpcrun_nanotime_clock(CLOCK_REALTIME);

  level0_check_result(status, __LINE__);

  // NOTES:
  // 1. Level Zero's host time is not comparable to host CLOCK_REALTIME.
  //    According to PTI documentation, it is CLOCK_MONOTONIC_RAW.
  // 2. HPCToolkit uses CLOCK_REALTIME in traces.
  //
  // As the result of these facts, I substitute host CLOCK_REALTIME for
  // the host time in the synchronized pair to provide a basis for
  // translating device times to times consistent with host CLOCK_REALTIME.
  // -- John Mellor-Crummey 11/4/2025

  *host_time = real;

  PRINT("sync host = %ld sync device = %ld\n", *host_time, *device_time);
}


static uint64_t
level0_timer_resolution_in_ns
(
  ze_device_properties_t *props
)
{
  uint64_t resolution = 0;
  if (props->stype == ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES) {
    // timerResolution is in nanoseconds
    resolution = props->timerResolution;
  } else if (props->stype == ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2) {
    // timerResolution is in cycles/second
    // ns/s / cycles/s = ns/cycle
    resolution = NS_PER_SECOND / props->timerResolution;
  } else {
    fprintf(stderr, "FATAL: hpcrun: level0: unknown time resolution\n");
    exit(-1);
  }

  return resolution;
}

void
level0_device_properties_init
(
  ze_device_handle_t hDevice,
  const struct hpcrun_foil_appdispatch_level0 *dispatch,
  level0_device_properties_t *properties
)
{
  // Get the device properties from Level Zero.
  properties->device_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  properties->device_props.pNext = NULL;
  ze_result_t status = f_zeDeviceGetProperties(hDevice, &properties->device_props, dispatch);
  level0_check_result(status, __LINE__);

  // record recent host and device timestamps
  level0_update_timestamps(hDevice, dispatch, properties);

  // compute device timer resolution
  properties->device_timestamp_resolution_ns =
    level0_timer_resolution_in_ns(&properties->device_props);
}



//*****************************************************************************
// interface operations
//*****************************************************************************

level0_device_properties_t *
level0_device_properties_get
(
 ze_device_handle_t hDevice,
 const struct hpcrun_foil_appdispatch_level0 *dispatch
)
{
  uintptr_t device = (uintptr_t) hDevice;
  spinlock_lock(&map_lock);

  level0_device_properties_map_entry_t *result =
    level0_device_properties_map_lookup(&map_root, device);

  if (result == NULL) {
    // Create a record for caching device properties.
    result = level0_device_properties_map_entry_new(&free_list, device);

    level0_device_properties_init(hDevice, dispatch, &result->l0props);

    // Add the record to the cache (map).
    level0_device_properties_map_insert(&map_root, result);
  } else {
    // NOTES:
    // Reportedly, the GPU device clock frequency isn't necessarily stable and
    // clock drift is a concern. Accordingly, we follow Intel PTI-GPU's lead
    // and always return a recent synchronized pair as the basis for
    // interpolating from a GPU time to a host time. If a synchronized
    // (host, device) time pair is too old, fetch a new one.
    uint64_t current_host_time = hpcrun_nanotime();
    uint64_t synchronized_pair_age =
      current_host_time - result->l0props.recent_synchronized_timestamps.host;
    if (synchronized_pair_age > GPU_TIMESTAMP_VALIDITY_THRESHOLD_NS) {
      level0_update_timestamps(hDevice, dispatch, &result->l0props);
    }
  }

  spinlock_unlock(&map_lock);

  return &result->l0props;
}
