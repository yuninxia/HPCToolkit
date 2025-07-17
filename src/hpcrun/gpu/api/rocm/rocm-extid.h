// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause


/// @file rocm-extid.h
/// @brief A simple API for managing external correlation IDs for ROCm GPU operations.

#ifndef rocm_extid_h
#define rocm_extid_h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdint.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "rocm.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Initializes the ROCm external ID management.
/// @param context_id The ROCm context ID.
void
rocm_extid_init
(
  rocprofiler_context_id_t context_id
);

#endif  // rocm_extid_h
