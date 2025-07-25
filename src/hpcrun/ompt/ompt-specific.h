// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//----------------------------------------------------------------------

#ifndef _ompt_specific_h_
#define _ompt_specific_h_

#include "ompt-types.h"
#include "ompt-thread.h"
#include "../cct/cct.h"
#include "../tls_specific.h"

struct ompt_tls_data;

#define OMPT_GET(field)  (((struct ompt_tls_data *) TLS_GET(ompt_specific))->field)

#define OMPT_GET_BASE_PTR()  ((void *) TLS_GET(ompt_specific))

#define OMPT_BASE_GET(base, field)  (((struct ompt_tls_data *) (base))->field)

//----------------------------------------------------------------------

// The __thread local variables for ompt.

struct ompt_tls_data {

  // ompt-callstack.c
  int ompt_callstack_debug;

  // ompt-device.c
  bool ompt_need_flush;
  bool ompt_runtime_api_flag;
  cct_node_t * target_node;
  cct_node_t * trace_node;

  // ompt-interface.c
  // this variable holds a count of how many times the current thread
  // has been marked as idle. a count is used rather than a flag to
  // support nested marking.
  int ompt_idle_count;

  // ompt-region.c
  // private freelist from which only thread owner can reused regions
  ompt_data_t * private_region_freelist_head;

  // ompt-thread.c
  // public thread's notification queue
  ompt_wfq_t threads_queue;

  // private thread's notification queue
  ompt_data_t * private_threads_queue;

  // freelists
  // thread's list of notification that can be reused
  ompt_notification_t * notification_freelist_head;

  // thread's list of region's where thread was registered and resolved them
  ompt_trl_el_t * thread_region_freelist_head;

  // region's free lists
  // public freelist where all threads can enqueue region_data to be reused
  ompt_wfq_t public_region_freelist;

  // stack that contains all nested parallel region
  // FIXME vi3: 128 levels are supported (and not checked)
  region_stack_el_t region_stack[MAX_NESTING_LEVELS];
  int top_index;

  // Memoization process vi3:
  ompt_region_data_t * not_master_region;
  cct_node_t * cct_not_master_region;

  // FIXME vi3: just a temp solution
  ompt_region_data_t * ending_region;

  // number of unresolved regions
  int unresolved_cnt;

  int ompt_thread_type;
};

#endif  // _ompt_specific_h_
