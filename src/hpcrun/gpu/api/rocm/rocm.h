// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

/// @file rocm.h
/// @brief This file contains the interface for the ROCm API interaction.

#ifndef rocm_h
#define rocm_h

//******************************************************************************
// system includes
//******************************************************************************

#include <stdlib.h>



//******************************************************************************
// rocprofiler includes
//******************************************************************************

#include "rocprofiler.h"



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../foil/rocm.h"
#include "../../../messages/messages.h"



//******************************************************************************
// public interfaces
//******************************************************************************

/// @brief Macro to call a ROCprofiler function and check the return status.
//
/// This macro calls the specified ROCprofiler function, checks the status code,
/// and exits with an error message if the call fails.
///
/// @param fn The name of the ROCprofiler function.
/// @param args The arguments to the ROCprofiler function.
/// @param msg A message describing the operation being performed.
/// @return The status returned by the ROCprofiler function.
#define ROCPROFILER_CALL(fn, args, msg) /* return status using GNU ext */     \
({                                                                            \
   rocprofiler_status_t __status = f_ ## fn args;                             \
   TMSG(ROCM, "hpcrun: rocm foil call %s = %d", "f_" #fn #args, __status);    \
   if (__status != ROCPROFILER_STATUS_SUCCESS) {                              \
     const char *__status_msg = f_rocprofiler_get_status_string(__status);    \
     TMSG(ROCM, "hpcrun: rocprofiler failure '%s'"                            \
          " status = %d, status_msg = '%s'", msg, __status, __status_msg);    \
     fprintf(stderr, "hpcrun: rocprofiler failure '%s' "                      \
             " status = %d, status_msg = '%s'\n", msg, __status,              \
             __status_msg);                                                   \
     exit(-1);                                                                \
  }                                                                           \
  __status;                                                                   \
})


/// @brief Macro to call a ROCprofiler function and return status.
/// @param fn The name of the ROCprofiler function.
/// @param args The arguments to the ROCprofiler function.
/// @param msg A message describing the operation being performed.
/// @return return code from rocprofiler function call invocation.
#define ROCPROFILER_CALL_WITH_STATUS(fn, args, msg)                           \
({                                                                            \
   rocprofiler_status_t  __status = f_ ## fn args;                            \
   __status;                                                                  \
})

#endif
