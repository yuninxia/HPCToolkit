
// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../libmonitor/monitor.h"
#include "../../api/common/gpu-kernel-table.h"

#include "rocm.h"
#include "rocm-agent-profile-map.h"
#include "rocm-api.h"
#include "rocm-buffer.h"
#include "rocm-configure.h"
#include "rocm-codeobject.h"
#include "rocm-codeobject-map.h"
#include "rocm-context.h"
#include "rocm-counters.h"
#include "rocm-extid.h"
#include "rocm-interface.h"
#include "rocm-ompt.h"
#include "rocm-pauseresume.h"
#include "rocm-sampling.h"
#include "rocm-symbol-map.h"
#include "rocm-threads.h"



//******************************************************************************
// macros
//******************************************************************************

#define ROCM_STATUS_OK    (0)
#define ROCM_STATUS_ERROR (-1)



//******************************************************************************
// global variables
//******************************************************************************

hpctoolkit_tool_data_t rocprofiler_tool_data;

bool rocm_configure_started = false;



//******************************************************************************
// private operations
//******************************************************************************

static void
rocm_extract_version_info
(
  uint32_t version,
  uint32_t *major,
  uint32_t *minor,
  uint32_t *patch
)
{
  *major = version / 10000;
  *minor = (version % 10000) / 100;
  *patch = version % 100;
}


static void
rocm_tool_init_once_helper
(
  void
)
{
  TMSG(ROCM, "rocm_tool_init_once_helper");

  rocprofiler_context_id_t context_id = rocm_context_create();

  rocprofiler_tool_data.context_id = context_id;

  gpu_kernel_table_init();

  rocm_agent_profile_map_init();

  // maintain information about GPU code objects
  rocm_codeobject_init(context_id, rocprofiler_tool_data.buffer_id);

  // maintain a map of GPU code object ranges
  rocm_codeobject_map_init();

  rocm_symbol_map_init();

  // external correlation id API for associating GPU operations with calling contexts
  rocm_extid_init(context_id);

  // buffered API for profiling and tracing GPU operations
  rocm_api_init(context_id);

  rocm_ompt_init(context_id);

  // pause/resume control
  rocm_context_pause_resume_init(&rocprofiler_tool_data.context_id);

  // GPU hardware counters
  rocm_counters_init(context_id);

  // GPU PC sampling
  rocm_sampling_init(context_id);

  if (rocm_context_is_valid(context_id)) {
    rocm_context_start(context_id);
    rocm_interface_enable();
  } else {
    rocm_interface_disable();
  }
}


void
rocm_tool_init_once
(
  void
)
{
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, rocm_tool_init_once_helper);
}


static int
rocm_tool_init
(
  rocprofiler_client_finalize_t fini_func,
  void *tool_data
)
{
  TMSG(ROCM, "rocm_tool_init");

  rocm_configure_started = true;

  // force hpctoolkit initialization
  monitor_initialize();

  if (rocm_interface_is_enabled()) {
    rocm_tool_init_once();

    // post-condition: hpctoolkit is initialized for rocm

    rocprofiler_tool_data.client_fini = fini_func;
  }

  return rocm_interface_is_enabled();
}


static void
rocm_tool_fini_once
(
  void
)
{
  TMSG(ROCM, "rocm_tool_fini_once");

  if (rocm_interface_is_enabled()) {
    if (rocm_context_is_active(rocprofiler_tool_data.context_id)) {
      rocm_context_stop(rocprofiler_tool_data.context_id);
    }

    rocm_buffer_flush_all();
  }
}


static void
rocm_tool_fini
(
  void *tool_data
)
{
  TMSG(ROCM, "rocm_tool_fini");

  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, rocm_tool_fini_once);
}




//******************************************************************************
// interface operations
//******************************************************************************

// invoked when rocprofiler-sdk calls rocprofiler_configure
rocprofiler_tool_configure_result_t *
foilbase_rocprofiler_configure
(
  uint32_t version,
  const char *runtime_version,
  uint32_t priority,
  rocprofiler_client_id_t *id
)
{
  TMSG(ROCM, "foilbase_rocprofiler_configure");

  static rocprofiler_tool_configure_result_t rocprofiler_configuration = {
    sizeof(rocprofiler_tool_configure_result_t),
    &rocm_tool_init, &rocm_tool_fini, &rocprofiler_tool_data};

  // only activate if main tool
  if (priority > 0) return 0;

  // store client info
  rocprofiler_tool_data.client_id = (rocprofiler_client_id_t *) id;

  // set the client name
  rocprofiler_tool_data.client_id->name = "HPCToolkit";

  uint32_t major, minor, patch;
  rocm_extract_version_info(version, &major, &minor, &patch);

  TMSG(ROCM, "rocprofiler sdk version (major=%d, minor=%d, patch=%d)",
       major, minor, patch);

  rocm_threads_ignore_rocm_threads(&rocprofiler_tool_data);

  return (void *) &rocprofiler_configuration;
}


// invoked to force initialization of rocprofiler-sdk and HPCToolkit
// ROCm support
void
rocm_configure
(
  void
)
{
  TMSG(ROCM, "enter rocm_configure");
  if (rocm_configure_started == false) {
    rocm_configure_started = true;

    TMSG(ROCM, "rocm_configure: invoke rocprofiler_force_configure");

    ROCPROFILER_CALL
    (
      rocprofiler_force_configure, (foilbase_rocprofiler_configure),
      "rocprofiler_force_configure"
    );
  } else {
    rocm_tool_init_once();
  }
}


void
rocm_configure_fini
(
  void
)
{
  rocm_tool_fini(0);
}
