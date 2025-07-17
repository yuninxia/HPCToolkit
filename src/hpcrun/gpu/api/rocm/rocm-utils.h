// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm-utils.h
/// @brief Utilities for extracting names, correlation ID, etc

#ifndef rocm_utils_h
#define rocm_utils_h

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

/// @brief Get the name of a buffer category.
/// @param category The buffer category.
/// @return The name of the buffer category.
const char *
rocm_get_buffer_category_name
(
  uint32_t category
);


/// @brief Get the name of a buffer kind.
/// @param kind The buffer kind.
/// @return The name of the buffer kind.
const char *
rocm_get_buffer_kind_name
(
  uint32_t kind
);


rocprofiler_status_t
rocm_get_buffer_kind_operation_name
(
  rocprofiler_buffer_tracing_kind_t kind,
  rocprofiler_tracing_operation_t operation,
  const char **name,
  uint64_t* name_len
);


/// @brief Get the name of a pc sampling kind.
/// @param kind The pc sampling kind.
/// @return The name of the pc sampling kind.
const char *
rocm_get_pc_sampling_kind_name
(
  uint32_t kind
);


/// @brief Get the external id from a rocprofiler record header.
/// @param header The rocprofiler record header.
/// @return The external id.
uint64_t
rocm_get_extern_id
(
  rocprofiler_record_header_t * header
);


#endif  // rocm_utils_h
