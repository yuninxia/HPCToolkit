// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//----------------------------------------------------------------------

#ifndef _tls_specific_h_
#define _tls_specific_h_

#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>

#include "memory/newmem.h"
#include "sample-sources/pthread-blame.h"
#include "unwind/common/binarytree_uwi.h"

struct tls_data;

typedef struct tls_data hpcrun_tls_data_t;

pthread_key_t hpcrun_get_tls_key(void);
void hpcrun_tls_specific_init_process(void);
void hpcrun_tls_specific_init_thread(void);

static inline void *
lazy_getspecific(void)
{
  void * spec = pthread_getspecific(hpcrun_get_tls_key());

  if (spec != NULL) {
    return spec;
  }
  hpcrun_tls_specific_init_thread();

  return pthread_getspecific(hpcrun_get_tls_key());
}

#define TLS_GETSPECIFIC(field)  \
  (& (((struct tls_data *) lazy_getspecific())->field))

//----------------------------------------------------------------------

// TLS Data Struct -- this contains all the __thread local variables
// outside of thread_data_t

struct tls_data {
  // main.c
  bool suppress_sample;

  // sample_sources_all.c
  int ignore_thread;

  // thread_data.c
  int  monitor_tid;
  bool mem_pool_initialized;

  // trace.c
  uint64_t prev_nanotime;

  // cct/cct.c -- really, 'cct_node_t *', but that pulls in too much
  // and generates an error
  void * cct_node_freelist_head;

  // memory/mem.c
  hpcrun_meminfo_t memstore;
  int mem_low;

  // sample-sources/itimer.c
  bool wallclock_ok;

  // sample-sources/papi-c-intel.c
  bool event_set_created;
  bool event_set_finalized;
  int  my_event_set;

  // sample-sources/pthread-blame.c
  blame_t pthread_blame;

  // sample-sources/perf/kernel_blocking.c
  uint64_t perf_time_cs_out;
  void *   perf_cct_kernel;
  uint32_t perf_cpu;
  uint32_t perf_pid;
  uint32_t perf_tid;

  // skiplist/cskiplist.c
  void * lf_cskl_nodes;

  // skiplist/urand.c
  int urand_initialized;
  unsigned int urand_data;

  // unwind/common/binarytree_uwi.c -- really 'bitree_uwi_t * [NUM_UNWINDERS]'
  void * _lf_uwi_tree[NUM_UNWINDERS];

  // unwind/common/uw_recipe_map.c -- really 'ilmstat_btuwi_pair_t *'
  void * _lf_ilmstat_btuwi;
};

#endif  // _tls_specific_h_
