// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause


// Utilities for extracting names, correlation ID, etc.
// Mostly for debugging.

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../messages/messages.h"
#include "rocm-utils.h"



//******************************************************************************
// private variables
//******************************************************************************

static const char * name_unknown = "unknown";



//******************************************************************************
// public interfaces
//******************************************************************************

//
// Return buffer category name, apparently there isn't one in
// rocprofiler.
//
const char *
rocm_get_buffer_category_name
(
  uint32_t category
)
{
    if (category == ROCPROFILER_BUFFER_CATEGORY_TRACING) {
        return "TRACING";
    }
    else if (category == ROCPROFILER_BUFFER_CATEGORY_PC_SAMPLING) {
        return "PC_SAMPLING";
    }
    else if (category == ROCPROFILER_BUFFER_CATEGORY_COUNTERS) {
        return "COUNTERS";
    }

    TMSG(ROCM, "record header category out of bounds: %d", category);

    return name_unknown;
}


//
// Lookup buffer tracing kind name.
//
const char *
rocm_get_buffer_kind_name
(
  uint32_t kind
)
{
  const char *name = NULL;

  if (kind <= ROCPROFILER_BUFFER_TRACING_NONE
      || kind >= ROCPROFILER_BUFFER_TRACING_LAST) {
    TMSG(ROCM, "record header kind out of bounds: %d", kind);
    return name_unknown;
  }

  ROCPROFILER_CALL(
    rocprofiler_query_buffer_tracing_kind_name,
    (kind, &name, NULL),
    "rocprofiler_query_buffer_tracing_kind_name"
  );

  return name;
}


rocprofiler_status_t
rocm_get_buffer_kind_operation_name
(
  rocprofiler_buffer_tracing_kind_t kind,
  rocprofiler_tracing_operation_t operation,
  const char **name,
  uint64_t* name_len
)
{
  rocprofiler_status_t status =
    ROCPROFILER_CALL_WITH_STATUS
    (
      rocprofiler_query_buffer_tracing_kind_operation_name,
        (kind, operation, name, name_len),
      "rocprofiler_query_buffer_tracing_kind_operation_name"
    );
  return status;
}


//
// Lookup pc-sampling kind name.
//
const char *
rocm_get_pc_sampling_kind_name
(
  uint32_t kind
)
{
  switch(kind) {
  #ifdef ROCPROFILER_PC_SAMPLING_RECORD_HOST
    case ROCPROFILER_PC_SAMPLING_RECORD_HOST:
      return "SAMPLE: HOST_TRAP";
  #endif
  #ifdef ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC
    case ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC:
      return "SAMPLE: STOCHASTIC";
  #endif
    default: break;
  }

  return name_unknown;
}


//
// Extract external correlation ID from record header based on "kind"
// in header.  This only applies to buffer tracing headers, not
// pc-sampling or counters.
//
uint64_t
rocm_get_extern_id
(
  rocprofiler_record_header_t * header
)
{
  uint32_t category = header->category;
  uint32_t kind = header->kind;
  void * payload = header->payload;
  uint64_t extid = 0;

  if (category != ROCPROFILER_BUFFER_CATEGORY_TRACING) {
    return 0;
  }

  if (kind <= ROCPROFILER_BUFFER_TRACING_NONE
      || kind >= ROCPROFILER_BUFFER_TRACING_LAST) {
    TMSG(ROCM, "record header kind out of bounds: %d", kind);
    return 0;
  }

  switch (kind) {
    //
    // These three have no external correlation ID.
    //
  #ifdef ROCPROFILER_PAGE_MIGRATION_AVAILABLE
    case ROCPROFILER_BUFFER_TRACING_PAGE_MIGRATION:
  #endif
    case ROCPROFILER_BUFFER_TRACING_SCRATCH_MEMORY:
    case ROCPROFILER_BUFFER_TRACING_CORRELATION_ID_RETIREMENT:
      break;

    //
    // For all others, the struct differs, but the placement is
    // the same (I think).
    //
    default:
      extid = ((rocprofiler_buffer_tracing_hip_api_record_t *) payload)->correlation_id.external.value;
      break;
    }

  return extid;
}
