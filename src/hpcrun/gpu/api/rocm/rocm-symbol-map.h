// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-symbol-map.h
/// @brief This file contains the interface for the ROCm symbol map.

#ifndef rocm_symbol_map_h
#define rocm_symbol_map_h

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../utilities/ip-normalized.h"

#include "rocm.h"



//******************************************************************************
// type declarations
//******************************************************************************

/// @brief Type definition for a ROCm kernel symbol.
typedef rocprofiler_callback_tracing_code_object_kernel_symbol_register_data_t
  rocprofiler_kernel_symbol_t;

/// @brief Structure to store information about a ROCm kernel symbol.
/// @brief Normalized IP of the kernel.
/// @brief Number of bytes of kernel arguments.
/// @brief Number of bytes of workgroup LDS.
/// @brief Number of bytes of kernel scratch memory.
typedef struct rocm_symbol_info_t {
  ip_normalized_t kernel_ip;
  uint32_t kernel_arg_bytes;
  uint32_t workgroup_LDS_bytes;
  uint32_t kernel_scratch_bytes;
  uint32_t kernel_sgpr_count;
  uint32_t kernel_vgpr_count;
  uint32_t total_vgpr_count;
} rocm_symbol_info_t;



//******************************************************************************
// interface operations
//******************************************************************************

/// @brief Initializes the ROCm symbol map.
void
rocm_symbol_map_init
(
  void
);


/// @brief Inserts a kernel symbol into the ROCm symbol map.
/// @param kernel_symbol_data Pointer to the kernel symbol data.
void
rocm_symbol_map_insert
(
  rocprofiler_kernel_symbol_t *kernel_symbol_data
);


/// @brief Deletes a kernel symbol from the ROCm symbol map.
/// @param kernel_symbol_data Pointer to the kernel symbol data to delete.
/// @return True if the symbol was deleted, false otherwise.
bool
rocm_symbol_map_delete
(
  rocprofiler_kernel_symbol_t *kernel_symbol_data
);


/// @brief Finds a kernel symbol in the ROCm symbol map.
/// @param kernel_id The ID of the kernel to find.
/// @return A pointer to the kernel symbol information, or NULL if not found.
rocm_symbol_info_t *
rocm_symbol_map_find
(
  rocprofiler_kernel_id_t kernel_id
);

#endif
