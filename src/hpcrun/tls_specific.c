// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//----------------------------------------------------------------------

// This file reimplements the TLS __thread local storage in hpcrun
// with pthread_get/setspecific().
//
// __thread is not signal handler safe.  In particular, GNU glibc can
// cause __thread to call realloc() in the signal handler.
//
// Neither is get/setspecific, but here we only use set-specific at a
// safe point early in the pthread start-routine and only use
// get-specific inside a handler.  Experimentally, this seems to work,
// at least The Machine has failed to push it over.
//
// We move all of the __thread variables into a single tls_data struct
// (per thread) and then TLS_GETSPECIFIC(field) returns a pointer to
// 'field' in the struct.  Much like TD_GET, except here we return
// pointer to field.
//
//----------------------------------------------------------------------
//
// To use this file, replace a TLS variable, for example:
//
//   __thread int foo;
//
// with an entry in 'struct tls_data' in tls_specific.h (without
// __thread).  Then, replace using 'foo' with a pointer to foo.
//
//   int *foo = TLS_GETSPECIFIC(foo);
//
// Note:
// (1) The storage size of the entry in 'struct tls_data' must match
// the original __thread storage size.  For simple types (int, long,
// void*), this is not a problem.  For a struct (but not pointer to
// struct), the full struct must appear in tls_data, and the header
// files to define the struct must be in tls_specific.h
//
// (2) For a pointer to a complex type, one option is to use 'void *'
// in tls_data and then cast to the correct type when extracting the
// variable.
//
// (3) Initialization is done in hpcrun_init_tls_data() at the bottom
// of this file.  But you really only need to initialize the non-zero
// values.
//
// (4) Remember to include "tls_specific.h" in the files that use
// TLS_GETSPECIFIC().
//
// (5) The gpu files are not used in a signal handler, so they don't
// need to be converted (johnmc assures me).  But they do use TMSG
// which uses monitor_tid, so their threads need a tls_data struct.

//----------------------------------------------------------------------

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include "tls_specific.h"
#include "messages/messages.h"
#include "sample-sources/pthread-blame.h"

#ifdef HPCRUN_SS_PAPI
#include <papi.h>
#else
#define PAPI_NULL  (-1)
#endif

// fixme: copied from sample_sources_all.c
#define THREAD_DOINIT  0

static pthread_once_t tls_once = PTHREAD_ONCE_INIT;
static pthread_key_t tls_key;

static void hpcrun_init_tls_data(hpcrun_tls_data_t *);

void * hpcrun_ompt_alloc_specific(void);

//----------------------------------------------------------------------

// Internal Functions (except init data)

// cleanup tls_data struct.  called at thread exit with the
// set-specific value.
//
static void
tls_fini_thread(void * data)
{
  free(((struct tls_data *) data)->ompt_specific);
  free(data);
}

static void
tls_once_fcn(void)
{
  if (pthread_key_create(&tls_key, tls_fini_thread) != 0) {
    hpcrun_abort("hpcrun: pthread_key_create(tls_key) failed");
  }
  hpcrun_tls_specific_init_thread();
}

//----------------------------------------------------------------------

// API Functions

pthread_key_t
hpcrun_get_tls_key(void)
{
  return tls_key;
}

// pthread_key_create() once for the process.
// may be called from multiple possible entry points
//
void
hpcrun_tls_specific_init_process(void)
{
  if (pthread_once(&tls_once, tls_once_fcn) != 0) {
    hpcrun_abort("hpcrun: pthread_once(tls_once_fcn) failed");
  }
}

// pthread_setspecific() for each thread.
// called once per thread from pthread start routine
//
void *
hpcrun_tls_specific_init_thread(void)
{
  hpcrun_tls_data_t * data = malloc(sizeof(hpcrun_tls_data_t));

  if (data == NULL) {
    hpcrun_abort("hpcrun: malloc(tls specific) failed");
  }

  hpcrun_init_tls_data(data);

  if (pthread_setspecific(tls_key, data) != 0) {
    hpcrun_abort("hpcrun: pthread_setspecific() failed");
  }

  return data;
}

//----------------------------------------------------------------------

// Initialize the thread local variables

// really only need to init non-zero values here
//
static void
hpcrun_init_tls_data(hpcrun_tls_data_t *data)
{
  memset(data, 0, sizeof(hpcrun_tls_data_t));

  data->ompt_specific = hpcrun_ompt_alloc_specific();
  data->suppress_sample = true;
  data->ignore_thread = THREAD_DOINIT;
  data->monitor_tid = -1;
  data->mem_pool_initialized = false;
  data->prev_nanotime = 0;
  data->cct_node_freelist_head = NULL;
  data->memstore = (hpcrun_meminfo_t) { NULL, NULL, NULL, 0 };
  data->mem_low = 0;
  data->wallclock_ok = false;
  data->event_set_created = false;
  data->event_set_finalized = false;
  data->my_event_set = PAPI_NULL;
  data->pthread_blame = (blame_t) { 0, Running };
  data->perf_time_cs_out = 0;
  data->perf_cct_kernel = NULL;
  data->perf_cpu = 0;
  data->perf_pid = 0;
  data->perf_tid = 0;
  data->lf_cskl_nodes = NULL;
  data->urand_initialized = 0;
  data->urand_data = 0;
  data->lf_ilmstat_btuwi = NULL;
}
