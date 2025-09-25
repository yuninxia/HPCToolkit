// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-activity.h
/// @brief This file contains the interface for processing ROCm activities.
///
/// A GPU monitoring thread will process a ROCm activity measurement of a
/// GPU operation by translating it into a device-independent GPU activity
/// and then sending the resulting GPU activity back to the application thread
/// that initiated the GPU operation.


#ifndef rocm_activity_h
#define rocm_activity_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../gpu/activity/gpu-activity.h"

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Sends a ROCm GPU activity.
/// @param correlation_id The correlation ID specifying the originating thread to receive the activity.
/// @param gpu_activity The GPU activity to send to the originating thread
void
rocm_activity_send
(
  uint64_t correlation_id,
  gpu_activity_t *gpu_activity
);


/// @brief Processes a ROCm activity record.
/// @param rocprofiler_record A pointer to the ROCprofiler record header.
/// @return Correlation id (0 if nothing useful)
uint64_t
rocm_activity_process
(
 rocprofiler_record_header_t *rocprofiler_record
);

#endif
