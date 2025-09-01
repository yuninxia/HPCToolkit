// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../decl-init-cast.h"
#include "../../../libmonitor/monitor.h"
#include "../../../messages/messages.h"

#include "rocm-binaries.h"
#include "rocm-buffer.h"
#include "rocm-callback.h"
#include "rocm-codeobject.h"
#include "rocm-codeobject-map.h"
#include "rocm-context.h"
#include "rocm-symbol-map.h"
#include "rocm-threads.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// private interfaces
//******************************************************************************

static void
rocm_kernel_load
(
  rocprofiler_callback_tracing_record_t record,
  rocprofiler_kernel_code_object_t *kernel
)
{
  TMSG(ROCM, "load kernel");

  assert(record.phase == ROCPROFILER_CALLBACK_PHASE_LOAD);

  rocprofiler_kernel_code_object_t *code_object =
    (rocprofiler_kernel_code_object_t *) record.payload;

  uint32_t lmid = rocm_binary_uri_add(code_object->uri);

  PRINT("loaded code object %s at [0x%lx, 0x%lx) delta=%ld\n",
        code_object->uri, code_object->load_base,
        code_object->load_size + code_object->load_base,
        code_object->load_delta);
  rocm_codeobject_map_insert(code_object, lmid);
}


static void
rocm_kernel_unload
(
  rocprofiler_callback_tracing_record_t record,
  rocprofiler_kernel_code_object_t *kernel
)
{
  TMSG(ROCM, "unload kernel");

  assert(record.phase == ROCPROFILER_CALLBACK_PHASE_UNLOAD);

  rocprofiler_kernel_code_object_t *code_object =
    (rocprofiler_kernel_code_object_t *) record.payload;

  PRINT("unloaded code object %s at [0x%lx, 0x%lx) delta=%ld\n",
        code_object->uri, code_object->load_base,
        code_object->load_size + code_object->load_base,
        code_object->load_delta);

  // flush buffers to ensure that all lookups for kernel names in the code
  // object are complete before the code object is deleted from the map
  rocm_buffer_flush_all();

  rocm_codeobject_map_delete(code_object);
}


static void
rocm_codeobject_callback
(
  rocprofiler_callback_tracing_record_t record,
  rocprofiler_user_data_t *__unused__,
  void *callback_args
)
{
  if (record.kind == ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT) {
    switch(record.operation) {
      case ROCPROFILER_CODE_OBJECT_LOAD: {
        DECL_INIT_CAST(rocprofiler_kernel_code_object_t *,kernel, record.payload);
        switch(record.phase) {
          case ROCPROFILER_CALLBACK_PHASE_LOAD:
            rocm_kernel_load(record, kernel);
            break;
          case ROCPROFILER_CALLBACK_PHASE_UNLOAD:
            rocm_kernel_unload(record, kernel);
            break;
          default: break;
        }
        break;
      }
      case ROCPROFILER_CODE_OBJECT_DEVICE_KERNEL_SYMBOL_REGISTER: {
        DECL_INIT_CAST(rocprofiler_kernel_symbol_t *, kernel_symbol, record.payload);
        switch(record.phase) {
          case ROCPROFILER_CALLBACK_PHASE_LOAD:
            rocm_symbol_map_insert(kernel_symbol);
            break;
          case ROCPROFILER_CALLBACK_PHASE_UNLOAD:
            rocm_symbol_map_delete(kernel_symbol);
            break;
          default: break;
        }
        break;
      }
      default: break;
    }
  }
}



//******************************************************************************
// public interfaces
//******************************************************************************

void
rocm_codeobject_init
(
  rocprofiler_context_id_t context_id,
  rocprofiler_buffer_id_t buffer_id
)
{
  PRINT("rocm_codeobject_init: cid=0x%lx bid=0x%lx\n", rocm_context_id(context_id), rocm_buffer_id(buffer_id));

  rocm_callback_configure_initiation
    (context_id,
    ROCPROFILER_CALLBACK_TRACING_CODE_OBJECT, 0,0,
    rocm_codeobject_callback, &buffer_id,
    "code object tracing service configure");

  // Prepare for receiving a URI for a GPU kernel
  rocm_binary_uri_list_init();
}
