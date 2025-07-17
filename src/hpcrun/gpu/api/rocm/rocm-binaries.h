// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-binaries.h
/// @brief This file defines the interface for ingesting ROCm binaries and looking up functions in them.

#ifndef rocm_binaries_h
#define rocm_binaries_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../utilities/ip-normalized.h"



//******************************************************************************
// interface operations
//******************************************************************************

/// @brief Looks up a function in a ROCm binary.
/// @param device_id The device ID.
/// @param kernel_name The name of the kernel function.
/// @return The normalized IP of the function, or 0 if not found.
ip_normalized_t
rocm_binary_function_lookup
(
  int device_id,
  const char *kernel_name
);


/// @brief Adds a URI for a ROCm binary.
/// @param uri The URI of the binary.
/// @return A unique identifier for the URI.
uint32_t
rocm_binary_uri_add
(
  const char *uri
);


/// @brief Initializes the list of ROCm binary URIs.
void
rocm_binary_uri_list_init
(
  void
);

#endif
