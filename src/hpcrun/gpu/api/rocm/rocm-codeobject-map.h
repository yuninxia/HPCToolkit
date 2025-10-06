// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

/// @file rocm-codeobject-map.h
/// @brief Manages a map of ROCm code objects and their associated load modules.
///
/// This file provides interfaces for inserting, finding, deleting,
/// normalizing, and dumping ROCm code objects. It maintains a map
/// that associates code objects with load module identifiers,
/// which are essential for code location mapping.

#ifndef rocm_codeobject_map_h
#define rocm_codeobject_map_h

//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../utilities/ip-normalized.h"

#include "rocm.h"
#include "rocm-codeobject.h"



//*****************************************************************************
// type declarations
//*****************************************************************************

/// @brief Structure holding information about a ROCm code object.
///
/// This structure associates a `rocprofiler_kernel_code_object_t`
/// with the identifier of the load module it belongs to.
/// @brief Identifier for the load module associated with the code object.
/// @brief Identifier for the code object
typedef struct rocm_codeobject_info_t {
  rocprofiler_kernel_code_object_t code_object;
  uint32_t load_module_id;
} rocm_codeobject_info_t;



//*****************************************************************************
// interface operations
//*****************************************************************************

/// @brief Initializes the ROCm code object map.
///
/// This function must be called before any other operations on the code object map.
void
rocm_codeobject_map_init
(
  void
);


/// @brief Inserts a new code object into the map.
/// @param code_object Pointer to the code object to insert.
/// @param load_module_id The load module identifier associated with the code object.
/// @return True if the code object was successfully inserted, false otherwise.
bool
rocm_codeobject_map_insert
(
  rocprofiler_kernel_code_object_t *code_object,
  uint32_t load_module_id
);


/// @brief Finds a code object in the map based on a program counter (PC).
/// @param address The program counter to look up.
/// @return A pointer to the `rocm_codeobject_info_t` structure if found,
///         otherwise NULL.
rocm_codeobject_info_t *
rocm_codeobject_map_find
(
  rocprofiler_pc_t address
);


/// @brief Normalizes a ROCm program counter (PC) to an `ip_normalized_t`.
///
/// This function looks up the PC in the code object map to find
/// the associated load module and computes the offset of the PC within
/// that module.
///
/// @param address The program counter to normalize.
ip_normalized_t
rocm_codeobject_map_normalize
(
  rocprofiler_pc_t address
);


/// @brief Deletes a code object from the map.
/// @param code_object Pointer to the code object to delete.
/// @return True if the code object was successfully deleted, false
///         if it was not found.
bool
rocm_codeobject_map_delete
(
 rocprofiler_kernel_code_object_t *code_object
);


/// @brief Dumps the contents of the code object map for debugging.
///
/// This function prints the current contents of the code object map to stderr.
void
rocm_codeobject_map_dump
(
  void
);

#endif
