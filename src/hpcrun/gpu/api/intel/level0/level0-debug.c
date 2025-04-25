// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-debug.h"



//*****************************************************************************
// interface functions
//*****************************************************************************

const char *
ze_result_to_string
(
  ze_result_t result
)
{
#define R2S(r) case r: return #r;
  switch (result) {
    R2S(ZE_RESULT_SUCCESS)                                // v1.0
    R2S(ZE_RESULT_NOT_READY)                              // v1.0
    R2S(ZE_RESULT_ERROR_DEVICE_LOST)                      // v1.0
    R2S(ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY)               // v1.0
    R2S(ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY)             // v1.0
    R2S(ZE_RESULT_ERROR_MODULE_BUILD_FAILURE)             // v1.0
    R2S(ZE_RESULT_ERROR_MODULE_LINK_FAILURE)              // v1.0
    R2S(ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET)            // v1.0
    R2S(ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE)        // v1.0
    R2S(ZE_RESULT_EXP_ERROR_DEVICE_IS_NOT_VERTEX)         // v1.0
    R2S(ZE_RESULT_EXP_ERROR_VERTEX_IS_NOT_DEVICE)         // v1.0
    R2S(ZE_RESULT_EXP_ERROR_REMOTE_DEVICE)                // v1.0
  #if ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT) >= 1 && ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT) >= 7
    R2S(ZE_RESULT_EXP_ERROR_OPERANDS_INCOMPATIBLE)        // v1.7
    R2S(ZE_RESULT_EXP_RTAS_BUILD_RETRY)                   // v1.7
    R2S(ZE_RESULT_EXP_RTAS_BUILD_DEFERRED)                // v1.7
  #endif
    R2S(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS)         // v1.0
    R2S(ZE_RESULT_ERROR_NOT_AVAILABLE)                    // v1.0
    R2S(ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE)           // v1.0
    R2S(ZE_RESULT_WARNING_DROPPED_DATA)                   // v1.0
    R2S(ZE_RESULT_ERROR_UNINITIALIZED)                    // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_VERSION)              // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE)              // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_ARGUMENT)                 // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_NULL_HANDLE)              // v1.0
    R2S(ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE)             // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_NULL_POINTER)             // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_SIZE)                     // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_SIZE)                 // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT)            // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT)   // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_ENUMERATION)              // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION)          // v1.0
    R2S(ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT)         // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_NATIVE_BINARY)            // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_GLOBAL_NAME)              // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_KERNEL_NAME)              // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_FUNCTION_NAME)            // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION)     // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION)   // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX)    // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE)     // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE)   // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED)          // v1.0
    R2S(ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE)        // v1.0
    R2S(ZE_RESULT_ERROR_OVERLAPPING_REGIONS)              // v1.0
#if ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT) >= 1 && ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT) >= 4
    R2S(ZE_RESULT_WARNING_ACTION_REQUIRED)                // v1.4
#endif
#if ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT) >= 1 && ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT) >= 10
    R2S(ZE_RESULT_ERROR_INVALID_KERNEL_HANDLE)            // v1.10
#endif
#if ZE_MAJOR_VERSION(ZE_API_VERSION_CURRENT) >= 1 && ZE_MINOR_VERSION(ZE_API_VERSION_CURRENT) >= 13
    R2S(ZE_RESULT_EXT_RTAS_BUILD_RETRY)                   // v1.13
    R2S(ZE_RESULT_EXT_RTAS_BUILD_DEFERRED)                // v1.13
    R2S(ZE_RESULT_EXT_ERROR_OPERANDS_INCOMPATIBLE)        // v1.13
    R2S(ZE_RESULT_ERROR_SURVIVABILITY_MODE_DETECTED)      // v1.13
#endif
    R2S(ZE_RESULT_ERROR_UNKNOWN)                          // v1.0
    R2S(ZE_RESULT_FORCE_UINT32)                           // v1.0
  }
  return "ZE_RESULT_ERROR_UNKNOWN";
}
