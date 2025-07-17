// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-interface.h
/// @brief This file defines the interface for the ROCm GPU API.


#ifndef rocm_interface_h
#define rocm_interface_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../gpu/common/gpu-counter-set.h"



//******************************************************************************
// interface operations
//******************************************************************************

/// @brief Initialize the ROCm interface.
///
/// This function initializes the ROCm interface and sets up the necessary
/// structures for handling GPU counters.
///
/// @param gpu_counter_set A pointer to the GPU counter set.
void
rocm_interface_init
(
  gpu_counter_set_t *gpu_counter_set
);


/// @brief Enable the ROCm interface.
///
/// This function enables the ROCm interface for collecting GPU metrics.
void
rocm_interface_enable
(
  void
);


/// @brief Disable the ROCm interface.
///
/// This function disables the ROCm interface, halting GPU metric collection.
void
rocm_interface_disable
(
  void
);


/// @brief Check if the ROCm interface is enabled.
///
/// This function returns whether the ROCm interface is currently enabled.
///
/// @return True if the interface is enabled, false otherwise.
bool
rocm_interface_is_enabled
(
  void
);



/// @brief Flush the ROCm interface.
///
/// This function flushes any buffered data from the ROCm interface.
///
/// @param args Additional arguments for flushing.
/// @param how Specifies how to flush the data.
void
rocm_interface_flush
(
  void* args,
  int how
);


/// @brief Finalize the ROCm interface.
///
/// This function finalizes the ROCm interface, cleaning up any allocated resources.
///
/// @param args Additional arguments for finalization.
/// @param how Specifies how to finalize.
void
rocm_interface_fini
(
  void* args,
  int how
);

#endif
