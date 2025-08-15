// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   cuda-api.c
//
// Purpose:
//   wrapper around NVIDIA CUDA layer
//
//***************************************************************************


//*****************************************************************************
// system include files
//*****************************************************************************

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>    // memset

#include <cuda_runtime.h>



//*****************************************************************************
// local include files
//*****************************************************************************

#include "../../../messages/messages.h"

#include "cuda-api.h"

#include "../../../foil/nvidia.h"



//*****************************************************************************
// macros
//*****************************************************************************
#define HPCRUN_CUDA_API_CALL(fn, args)                              \
{                                                                   \
  CUresult error_result = f_ ## fn args;                            \
  if (error_result != CUDA_SUCCESS) {                               \
    fatal_CUresult(#fn, error_result);                              \
  }                                                                 \
}


#define HPCRUN_CUDA_RUNTIME_CALL(fn, args)                          \
{                                                                   \
  cudaError_t error_result = f_ ## fn args;                         \
  if (error_result != cudaSuccess) {                                \
    fatal_cudaError_t(#fn, error_result);                           \
  }                                                                 \
}


//----------------------------------------------------------------------
// device capability
//----------------------------------------------------------------------

#define COMPUTE_MAJOR_TURING    7
#define COMPUTE_MINOR_TURING    5

#define DEVICE_IS_TURING(major, minor)      \
  ((major == COMPUTE_MAJOR_TURING) && (minor == COMPUTE_MINOR_TURING))


//----------------------------------------------------------------------
// runtime version
//----------------------------------------------------------------------

#define CUDA11 11

// according to https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART____VERSION.html,
// CUDA encodes the runtime version number as (1000 * major + 10 * minor)

#define RUNTIME_MAJOR_VERSION(rt_version) (rt_version / 1000)
#define RUNTIME_MINOR_VERSION(rt_version) ((rt_version % 100) / 10)


//******************************************************************************
// static data
//******************************************************************************




//******************************************************************************
// private operations
//******************************************************************************

static void
fatal_cudaError_t(const char *fn, cudaError_t error_result)
{
  const char *error_string = f_cudaGetErrorString(error_result);
  fprintf(stderr, "FATAL: hpcrun CUDA error: function %s failed with "
    "error code %d (error string '%s')\n", fn, error_result, error_string);
  exit(-1);
}

static void
fatal_CUresult(const char *fn, CUresult error_result)
{
  const char* error_string = f_cuGetErrorString(error_result);
  fprintf(stderr, "FATAL: hpcrun CUDA Driver API error: function %s failed with "
    "error code %d (error string '%s')\n", fn, error_result, error_string);
  exit(-1);
}


static int
cuda_device_sm_blocks_query
(
 int major,
 int minor
)
{
  switch(major) {
  case 7:
  case 6:
    return 32;
  default:
    // TODO(Keren): add more devices
    return 8;
  }
}



// returns 0 on success
static int
cuda_device_compute_capability
(
  int device_id,
  int *major,
  int *minor
)
{
  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device_id));

  return 0;
}


// returns 0 on success
static int
cuda_device_id
(
  int *device_id
)
{
  HPCRUN_CUDA_RUNTIME_CALL(cudaGetDevice, (device_id));
  return 0;
}


static void
cuda_runtime_version
(
  int *rt_version
)
{
  HPCRUN_CUDA_RUNTIME_CALL(cudaRuntimeGetVersion, (rt_version));
}



//******************************************************************************
// interface operations
//******************************************************************************


int
cuda_context
(
 CUcontext *ctx
)
{
  HPCRUN_CUDA_API_CALL(cuCtxGetCurrent, (ctx));
  return 0;
}

int
cuda_device_property_query
(
 int device_id,
 cuda_device_property_t *property
)
{
  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->sm_count, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->sm_clock_rate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->sm_shared_memory,
     CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_MULTIPROCESSOR, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->sm_registers,
     CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->sm_threads, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR,
     device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&property->num_threads_per_warp, CU_DEVICE_ATTRIBUTE_WARP_SIZE,
     device_id));

  int major = 0, minor = 0;

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device_id));

  HPCRUN_CUDA_API_CALL(cuDeviceGetAttribute,
    (&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device_id));

  property->sm_blocks = cuda_device_sm_blocks_query(major, minor);

  return 0;
}


// return 0 on success
int
cuda_global_pc_sampling_required
(
  int *required
)
{
  int device_id;
  if (cuda_device_id(&device_id)) return -1;

  int dev_major, dev_minor;
  if (cuda_device_compute_capability(device_id, &dev_major, &dev_minor)) return -1;

  int rt_version;
  cuda_runtime_version(&rt_version);

#ifdef DEBUG
  printf("cuda_global_pc_sampling_required: "
         "device major = %d minor = %d cuda major = %d\n",
         dev_major, dev_minor, RUNTIME_MAJOR_VERSION(rt_version));
#endif

  *required = ((DEVICE_IS_TURING(dev_major, dev_minor)) &&
               (RUNTIME_MAJOR_VERSION(rt_version) < CUDA11));

  return 0;
}


int
cuda_get_module
(
 CUmodule *mod,
 CUfunction fn
)
{
  HPCRUN_CUDA_API_CALL(cuFuncGetModule, (mod, fn));
  return 0;
}


int
cuda_get_driver_version
(
 int *major,
 int *minor
)
{
  int version;

  HPCRUN_CUDA_API_CALL(cuDriverGetVersion, (&version));

  *major = version / 1000;
  *minor = version - *major * 1000;

  return 0;
}


int
cuda_get_runtime_version
(
 int *major,
 int *minor
)
{
  int rt_version;
  cuda_runtime_version(&rt_version);

  *major = RUNTIME_MAJOR_VERSION(rt_version);
  *minor = RUNTIME_MINOR_VERSION(rt_version);

  return 0;
}


int
cuda_get_code
(
 void* host,
 const void* dev,
 size_t bytes
)
{
  HPCRUN_CUDA_RUNTIME_CALL(cudaMemcpy, (host, dev, bytes, cudaMemcpyDeviceToHost));
  return 0;
}
