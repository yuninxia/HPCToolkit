// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef roctracer_activity_translate_h
#define roctracer_activity_translate_h

//******************************************************************************
// ROCm includes
//******************************************************************************

#include <roctracer/roctracer_hip.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "../../activity/gpu-activity.h"



//******************************************************************************
// interface functions
//******************************************************************************

void
roctracer_activity_translate
(
 gpu_activity_t *entry,
 roctracer_record_t *record,
 uint64_t *correlation_id
);



#endif
