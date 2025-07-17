// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm-buffer.h"
#include "rocm-bufferset.h"
#include "rocm-threads.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// private operations
//******************************************************************************

static bool
header_is_invalid
(
  rocprofiler_record_header_t *h
)
{
  uint64_t val = rocprofiler_record_header_compute_hash(h->category, h->kind);
  return (h->hash != val);
}




//******************************************************************************
// public operations
//******************************************************************************

rocprofiler_buffer_id_t
rocm_buffer_create
(
  rocprofiler_context_id_t context_id,
  size_t size,
  size_t watermark,
  rocprofiler_buffer_tracing_cb_t callback,
  rocprofiler_callback_thread_t callback_thread,
  void *user_data
)
{
  rocprofiler_buffer_id_t buffer_id;

  ROCPROFILER_CALL
  (
    rocprofiler_create_buffer,
    (
      context_id, size, watermark,
      ROCPROFILER_BUFFER_POLICY_LOSSLESS,
      callback, user_data,
      &buffer_id
    ),
    "buffer creation"
  );

  PRINT("buffer created = 0x%lx\n", buffer_id.handle);

  rocm_bufferset_insert(buffer_id);

  rocm_threads_assign_callback_thread(buffer_id, callback_thread);

  return buffer_id;
}


void
rocm_buffer_flush
(
  rocprofiler_buffer_id_t buffer_id
)
{
  ROCPROFILER_CALL
  (
    rocprofiler_flush_buffer, (buffer_id), "flush buffer"
  );
}


void
rocm_buffer_flush_all
(
  void
)
{
  rocm_bufferset_apply(rocm_buffer_flush);
}


void
rocm_buffer_process
(
  rocprofiler_record_header_t **headers,
  size_t num_headers,
  uint64_t drop_count,
  rocm_record_process_op_t record_process,
  void *user_data
)
{
  TMSG(ROCM, "rocm_buffer_process");

  if (drop_count != 0) {
    TMSG(ROCM, "rocprofiler: drop count should be zero for lossless policy");
  }

  // verify buffer contains record headers
  if (num_headers == 0 || headers == 0) {
    TMSG(ROCM, "rocprofiler invoked a buffer callback with no headers.");
    return;
  }

  // iterate over records
  for(size_t i = 0; i < num_headers; ++i) {
    rocprofiler_record_header_t *header = headers[i];
    if (header == 0) {
      TMSG(ROCM, "rocprofiler: null record header encountered.");
    } else if (header_is_invalid(header)) {
      TMSG(ROCM, "rocprofiler: bad header hash.");
    } else {
      record_process(header);
    }
  }
}


uint64_t
rocm_buffer_id
(
  rocprofiler_buffer_id_t buffer_id
)
{
  return buffer_id.handle;
}
