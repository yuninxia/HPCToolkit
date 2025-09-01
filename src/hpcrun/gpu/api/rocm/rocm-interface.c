// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../gpu-application-thread-api.h"

#include "rocm-buffer.h"
#include "rocm-configure.h"
#include "rocm-counters.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// private data
//******************************************************************************

static bool rocm_enabled = false;



//******************************************************************************
// interface operations
//******************************************************************************

void
rocm_interface_init
(
  gpu_counter_set_t *gpu_counter_set
)
{
  rocm_enabled = true;

  rocm_counters_wanted(gpu_counter_set);

  rocm_configure();
}


void
rocm_interface_enable
(
  void
)
{
  rocm_enabled = true;
}


void
rocm_interface_disable
(
  void
)
{
  rocm_enabled = false;
}


bool
rocm_interface_is_enabled
(
  void
)
{
  return rocm_enabled;
}


void
rocm_interface_flush
(
  void* args,
  int how
)
{
  if (rocm_enabled == false) return;

  rocm_buffer_flush_all();
  gpu_application_thread_process_activities();
}


void
rocm_interface_fini
(
  void* args,
  int how
)
{
  if (rocm_enabled == false) return;

  rocm_configure_fini();
  rocm_interface_flush(args, how);
}
