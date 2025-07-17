// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef ompt_gpu_api_h
#define ompt_gpu_api_h



//******************************************************************************
// OpenMP includes
//******************************************************************************

#include "../../../ompt/omp-tools.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
ompt_buffer_completion_notify
(
 void
);


void
ompt_activity_process
(
 ompt_record_ompt_t *record
);



#endif
