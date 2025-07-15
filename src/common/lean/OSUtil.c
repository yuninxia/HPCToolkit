// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   OS Utilities
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
// Author:
//   Nathan Tallent, John Mellor-Crummey, Rice University.
//
//***************************************************************************

//************************* System Include Files ****************************

#include <pthread.h>
#include <sys/types.h> // getpid()

#include <stdatomic.h>
#include <stdio.h> // snprintf
#include <stdlib.h>
#include <string.h>

#define __USE_XOPEN_EXTENDED // for gethostid()
#include <unistd.h>

//*************************** User Include Files ****************************

#include "OSUtil.h"
#include "linux_info.h"
#include <stdatomic.h>

//*************************** macros **************************

#define KERNEL_NAME_FORMAT    "%s." HOSTID_FORMAT

//*************************** Forward Declarations **************************

//***************************************************************************
//
//***************************************************************************

struct envvar_rank_entry {
  const char* local;
  const char* global;
};


static const char* const envvars_jobid[] = {
  "LSB_JOBID",  // LSF
  "COBALT_JOBID",  // Cobalt
  "PBS_JOBID_SHORT",  // PBS
  "PBS_JOBID",  // PBS
  "SLURM_JOB_ID",  // SLURM
  "JOB_ID",  // Sun Grid Engine
  "FLUX_JOB_ID",  // Flux
  NULL
};


static const struct envvar_rank_entry envvars_rank[] = {
  {"PMI_LOCAL_RANK", "PMI_RANK"},  // PMI layer
  {"OMPI_COMM_WORLD_LOCAL_RANK", "OMPI_COMM_WORLD_RANK"},  // OpenMPI
  {"MPI_LOCALRANKID", NULL},  // MPICH
  {"SLURM_LOCALID", "SLURM_PROCID"},  // SLURM
  {"JSM_NAMESPACE_LOCAL_RANK", "JSM_NAMESPACE_RANK"},  // LSF
  {"PALS_LOCAL_RANKID", "PALS_RANKID"},  // PBS Pro
  {"FLUX_TASK_LOCAL_ID", "FLUX_TASK_RANK"},  // Flux
  {NULL, NULL}
};



//***************************************************************************
// private data
//***************************************************************************

static pthread_once_t once_control = PTHREAD_ONCE_INIT;

static const char *memoized_jobid = 0;
static const char *memoized_local_rank = 0;
static long long memoized_rank = -1;
static uint32_t memoized_hostid = 0;

static atomic_uintptr_t memoized_libc_getenv = 0;

#define LIBC_GETENV ((libc_getenv_t) memoized_libc_getenv)

#define MEMOIZE_LIBC_GETENV(libc_getenv) \
  atomic_store(&memoized_libc_getenv, (uintptr_t) libc_getenv)

//***************************************************************************
// private operations
//***************************************************************************

static long long
parse_uint(const char* str)
{
  if(str == NULL || str[0] == '\0')
    return -1;

  char* end = NULL;
  long long result = strtoll(str, &end, 10);
  return *end == '\0' ? result : -1;
}


static void
memoize_local_rank(void)
{
  for (const struct envvar_rank_entry* envvar = envvars_rank;
       envvar->local != NULL || envvar->global != NULL; ++envvar) {
    if (envvar->local != NULL) {
      const char *rank = LIBC_GETENV(envvar->local);
      if (rank != NULL) {
        memoized_local_rank = rank;
        break;
      }
    }
  }
}


static void
memoize_jobid(void)
{
  for (const char * const *envvar = envvars_jobid; *envvar != NULL; ++envvar) {
    const char *jobid = LIBC_GETENV(*envvar);
    if (jobid != NULL) {
      memoized_jobid = jobid;
      break;
    }
  }
}


static void
memoize_rank(void)
{
  for (const struct envvar_rank_entry *envvar = envvars_rank;
      envvar->local != NULL || envvar->global != NULL; ++envvar) {
    if (envvar->global != NULL) {
      const char *rid = LIBC_GETENV(envvar->global);
      if (rid != NULL) {
        memoized_rank = parse_uint(rid);
        break;
      }
    }
  }
}


static void
memoize_hostid(void)
{
  // On many 64-bit systems `long == int64_t`, so the value out of gethostid()
  // may be sign-extended from the 32-bit hostid. This cast smashes out the
  // higher-order bits and makes it unsigned so it won't be sign-extended later.
  memoized_hostid = (uint32_t) gethostid();

  // We don't want to call gethostid() again in case it gives a different
  // result (happens sometimes on systems with bad network setups). So if it
  // gave 0, remap to something not 0. Like 1.
  if (memoized_hostid == 0) memoized_hostid = UINT32_C(1);
}


static void
memoize_environment(void)
{
  memoize_hostid();
  memoize_jobid();
  memoize_rank();
  memoize_local_rank();
}



//***************************************************************************
// interface operations
//***************************************************************************

unsigned int
OSUtil_pid()
{
  pid_t pid = getpid();
  return (unsigned int)pid;
}


const char *
OSUtil_jobid(libc_getenv_t libc_getenv)
{
  MEMOIZE_LIBC_GETENV(libc_getenv);
  pthread_once(&once_control, memoize_environment);
  return memoized_jobid;
}


const char *
OSUtil_local_rank(libc_getenv_t libc_getenv)
{
  MEMOIZE_LIBC_GETENV(libc_getenv);
  pthread_once(&once_control, memoize_environment);
  return memoized_local_rank;
}


long long
OSUtil_rank(libc_getenv_t libc_getenv)
{
  MEMOIZE_LIBC_GETENV(libc_getenv);
  pthread_once(&once_control, memoize_environment);
  return memoized_rank;
}


uint32_t
OSUtil_hostid(libc_getenv_t libc_getenv)
{
  MEMOIZE_LIBC_GETENV(libc_getenv);
  pthread_once(&once_control, memoize_environment);
  return memoized_hostid;
}


int
OSUtil_setCustomKernelName(char *buffer, size_t max_chars, libc_getenv_t libc_getenv)
{
  int n = snprintf(buffer, max_chars, KERNEL_NAME_FORMAT,
           LINUX_KERNEL_NAME_REAL, OSUtil_hostid(libc_getenv));

  return n;
}


int
OSUtil_setCustomKernelNameWrap(char *buffer, size_t max_chars, libc_getenv_t libc_getenv)
{
  int n = snprintf(buffer, max_chars, KERNEL_SYMBOLS_DIRECTORY "/" KERNEL_NAME_FORMAT,
           LINUX_KERNEL_NAME_REAL, OSUtil_hostid(libc_getenv));

  return n;
}
