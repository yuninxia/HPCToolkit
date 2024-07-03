// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef level0_command_process_h
#define level0_command_process_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-data-node.h"

//*****************************************************************************
// global variables
//*****************************************************************************

extern pthread_mutex_t gpu_activity_mtx;
extern pthread_cond_t cv;
extern bool data_processed;

extern pthread_mutex_t kernel_mutex;
extern pthread_cond_t kernel_cond;
extern int kernel_running;

//*****************************************************************************
// interface operations
//*****************************************************************************

void
level0_command_begin
(
  level0_data_node_t* command_node
);

void
level0_command_end
(
  level0_data_node_t* command_node,
  uint64_t start,
  uint64_t end
);

void
level0_flush_and_wait
(
  void
);

void
level0_wait_for_self_pending_operations
(
  void
);

#endif
