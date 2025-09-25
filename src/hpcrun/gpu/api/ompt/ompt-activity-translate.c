// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//******************************************************************************
// Description:
//   Read fields from a ompt_record_ompt_t and assign to a
//   GPU-independent gpu_activity_t.
//
//   This interface is only used by the CUPTI GPU monitoring thread.
//   It is thread-safe as long as it does not access details structures
//   shared by worker threads.
//******************************************************************************

//******************************************************************************
// local includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../../cct/cct.h"
#include "../../../cct/cct_addr.h"
#include "../../../utilities/ip-normalized.h"
#include "../../activity/gpu-activity.h"

#include "../rocm/rocm-interface.h"


#include "ompt-activity-translate.h"


//******************************************************************************
// macros
//******************************************************************************

#define OMPT_ACTIVITY_DEBUG 0

#if OMPT_ACTIVITY_DEBUG
#define PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif

#define FORALL_DATA_OPTYPES(macro) \
  macro(ompt_target_data_alloc) \
  macro(ompt_target_data_transfer_from_device) \
  macro(ompt_target_data_transfer_to_device) \
  macro(ompt_target_data_delete) \
  macro(ompt_target_data_associate) \
  macro(ompt_target_data_disassociate) \
  macro(ompt_target_data_transfer_to_device_async) \
  macro(ompt_target_data_transfer_from_device_async) \
  macro(ompt_target_data_alloc_async) \
  macro(ompt_target_data_delete_async)


//******************************************************************************
// forward declarations
//******************************************************************************

// unused unless OMPT_ACTIVITY_DEBUG is non-zero
static const char *
data_optype_string
(
  int optype
) __attribute__ ((unused)); 



//******************************************************************************
// private operations
//******************************************************************************

static void
convert_unknown
(
 ompt_device_map_entry_t *device_entry,
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ga->kind = GPU_ACTIVITY_UNKNOWN;
  *cid_ptr = 0;
}


static void
convert_ptrop
(
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ga->kind = GPU_ACTIVITY_UNKNOWN;
  *cid_ptr = 0;
}


static void
convert_target
(
 ompt_device_map_entry_t *device_entry,
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ompt_record_target_t *t __attribute__((unused)) = &r->record.target;

  ga->kind = GPU_ACTIVITY_UNKNOWN;
  *cid_ptr = 0;
}


static void
convert_memory
(
  gpu_activity_t *ga,
  ompt_record_ompt_t *r,
  gpu_mem_op_t mem_op,
  uint64_t *cid_ptr
)
{
  ompt_record_target_data_op_t *d = &r->record.target_data_op;

  ga->kind = GPU_ACTIVITY_MEMORY;
  ga->details.memory.memKind = GPU_MEM_UNKNOWN;
  ga->details.memory.correlation_id = d->host_op_id;
  ga->details.memory.mem_op = mem_op;
  *cid_ptr = d->host_op_id;

  ga->details.memory.bytes = d->bytes;
}


static void
convert_alloc
(
  gpu_activity_t *ga,
  ompt_record_ompt_t *r,
  uint64_t *cid_ptr
)
{
  convert_memory(ga, r, GPU_MEM_OP_ALLOC, cid_ptr);
}


static void
convert_delete
(
  gpu_activity_t *ga,
  ompt_record_ompt_t *r,
  uint64_t *cid_ptr
)
{
  convert_memory(ga, r, GPU_MEM_OP_DELETE, cid_ptr);
}


static gpu_memcpy_type_t
convert_memcpy_type
(
 ompt_target_data_op_t kind
)
{
  switch (kind) {
  case ompt_target_data_transfer_to_device_async:
  case ompt_target_data_transfer_to_device:
    return GPU_MEMCPY_H2D;

  case ompt_target_data_transfer_from_device_async:
  case ompt_target_data_transfer_from_device:
    return GPU_MEMCPY_D2H;

  default:
    return GPU_MEMCPY_UNK;
  }
}


static void
convert_memcpy
(
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ompt_record_target_data_op_t *d = &r->record.target_data_op;

  ga->kind = GPU_ACTIVITY_MEMCPY;

  ga->details.memcpy.correlation_id = d->host_op_id;
  *cid_ptr = d->host_op_id;

  ga->details.memcpy.bytes = d->bytes;
  ga->details.memcpy.copyKind = convert_memcpy_type(d->optype);
}


static const char *
data_optype_string
(
  int optype
)
{
  #define RETURN_STRING(x) case x: return #x;
  switch(optype) {
    FORALL_DATA_OPTYPES(RETURN_STRING)
  default:
    return "ompt_target_data_unknown_op_type";
  }
  return 0;
}


static void
convert_target_data_op
(
 ompt_device_map_entry_t *device_entry,
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ompt_record_target_data_op_t *d = &r->record.target_data_op;

  switch(d->optype) {

  case ompt_target_data_transfer_to_device:
  case ompt_target_data_transfer_from_device:
    convert_memcpy(ga, r, cid_ptr);
    break;

  case ompt_target_data_alloc_async:
  case ompt_target_data_alloc:
    convert_alloc(ga, r, cid_ptr);
    break;

  case ompt_target_data_delete_async:
  case ompt_target_data_delete:
    convert_delete(ga, r, cid_ptr);
    break;

  case ompt_target_data_associate:
  case ompt_target_data_disassociate:
    convert_ptrop(ga, r, cid_ptr);
    break;

  default:
    convert_unknown(device_entry, ga, r, cid_ptr);
    break;
  }

  PRINT("ompt_activity_time: start =%16lx, end =%16lx optype = %s\n", r->time, d->end_time, data_optype_string(d->optype));

  gpu_interval_set(&ga->details.interval,
    ompt_activity_time(device_entry, r->time),
    ompt_activity_time(device_entry, d->end_time));
}


void
convert_target_submit
(
 ompt_device_map_entry_t *device_entry,
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  ompt_record_target_kernel_t *k = &r->record.target_kernel;

  ga->kind = GPU_ACTIVITY_KERNEL;
  ga->details.kernel.kernel_first_pc = ip_normalized_NULL;
  ga->details.kernel.correlation_id = k->host_op_id;
  *cid_ptr = k->host_op_id;

  PRINT("ompt_activity_time: start =%16lx, end =%16lx optype = ompt_callback_target_submit_emi \n", r->time, k->end_time);

  gpu_interval_set(&ga->details.interval,
    ompt_activity_time(device_entry, r->time),
    ompt_activity_time(device_entry, k->end_time));
}



//******************************************************************************
// interface operations
//******************************************************************************

void
ompt_activity_translate
(
 ompt_device_map_entry_t *device_entry,
 gpu_activity_t *ga,
 ompt_record_ompt_t *r,
 uint64_t *cid_ptr
)
{
  memset(ga, 0, sizeof(gpu_activity_t));

  #ifdef USE_ROCM
    bool is_rocm_enabled = rocm_interface_is_enabled();
  #else
   bool is_rocm_enabled = false;
  #endif

  // if the rocprofiler-sdk interface is in use, it will cause problems
  // if we watch data operations and kernel operations. they will also
  // be reported to rocprofiler-sdk, causing us to double count operations
  // and time. we must turn this interface off because of AMD's unfortunate
  // design of rocprofiler-sdk.
  //
  // a second reason we need to skip observing data movement events with
  // ompt when using rocprofiler-sdk is that rocprofiler-sdk 6.4 corrupts
  // the timestamps for some OMPT data movement events. sigh.
  switch (r->type) {

  case ompt_callback_target:
  case ompt_callback_target_emi:

    convert_target(device_entry, ga, r, cid_ptr);
    break;

  case ompt_callback_target_data_op:
  case ompt_callback_target_data_op_emi:
    if (is_rocm_enabled) {
      convert_unknown(device_entry, ga, r, cid_ptr);
    } else {
      convert_target_data_op(device_entry, ga, r, cid_ptr);
    }
    break;

  case ompt_callback_target_submit:
  case ompt_callback_target_submit_emi:
    if (is_rocm_enabled) {
      convert_unknown(device_entry, ga, r, cid_ptr);
    } else {
      convert_target_submit(device_entry, ga, r, cid_ptr);
    }
    break;

  default:
    convert_unknown(device_entry, ga, r, cid_ptr);
    break;
  }


  cstack_ptr_set(&(ga->next), 0);
}
