// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
// local include files
//***************************************************************************
#define _GNU_SOURCE

#include "sample_event.h"
#include "disabled.h"

#include "memory/hpcrun-malloc.h"
#include "messages/messages.h"

#include <stdatomic.h>
#include "../common/lean/hpcrun-fmt.h"
#include "unwind/common/validate_return_addr.h"


//***************************************************************************
// local variables
//***************************************************************************

static atomic_long num_samples_total = 0;
static atomic_long num_samples_attempted = 0;
static atomic_long num_samples_blocked_async = 0;
static atomic_long num_samples_blocked_dlopen = 0;
static atomic_long num_samples_dropped = 0;
static atomic_long num_samples_segv = 0;
static atomic_long num_samples_partial = 0;
static atomic_long num_samples_yielded = 0;

static atomic_long num_unwind_intervals_total = 0;
static atomic_long num_unwind_intervals_suspicious = 0;

static atomic_long trolled = 0;
static atomic_long frames_total = 0;
static atomic_long trolled_frames = 0;
static atomic_long frames_libfail_total = 0;

static atomic_long acc_trace_records = 0;
static atomic_long acc_trace_records_dropped = 0;
static atomic_long acc_samples = 0;
static atomic_long acc_samples_dropped = 0;

//***************************************************************************
// interface operations
//***************************************************************************

void
hpcrun_stats_reinit(void)
{
  atomic_store_explicit(&num_samples_total, 0, memory_order_relaxed);
  atomic_store_explicit(&num_samples_attempted, 0, memory_order_relaxed);
  atomic_store_explicit(&num_samples_blocked_async, 0, memory_order_relaxed);
  atomic_store_explicit(&num_samples_blocked_dlopen, 0, memory_order_relaxed);
  atomic_store_explicit(&num_samples_segv, 0, memory_order_relaxed);

  atomic_store_explicit(&num_unwind_intervals_total, 0, memory_order_relaxed);
  atomic_store_explicit(&num_unwind_intervals_suspicious, 0, memory_order_relaxed);

  atomic_store_explicit(&trolled, 0, memory_order_relaxed);
  atomic_store_explicit(&frames_total, 0, memory_order_relaxed);
  atomic_store_explicit(&trolled_frames, 0, memory_order_relaxed);
  atomic_store_explicit(&frames_libfail_total, 0, memory_order_relaxed);

  atomic_store_explicit(&acc_trace_records, 0, memory_order_relaxed);
  atomic_store_explicit(&acc_trace_records_dropped, 0, memory_order_relaxed);

  atomic_store_explicit(&acc_samples, 0, memory_order_relaxed);
  atomic_store_explicit(&acc_samples_dropped, 0, memory_order_relaxed);
}


//-----------------------------
// samples total
//-----------------------------

void
hpcrun_stats_num_samples_total_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_total, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_total(void)
{
  return atomic_load_explicit(&num_samples_total, memory_order_relaxed);
}



//-----------------------------
// samples attempted
//-----------------------------

void
hpcrun_stats_num_samples_attempted_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_attempted, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_attempted(void)
{
  return atomic_load_explicit(&num_samples_attempted, memory_order_relaxed);
}



//-----------------------------
// samples blocked async
//-----------------------------

// The async blocks happen in the signal handlers, without getting to
// hpcrun_sample_callpath, so also increment the total count here.
void
hpcrun_stats_num_samples_blocked_async_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_blocked_async, 1L, memory_order_relaxed);
  atomic_fetch_add_explicit(&num_samples_total, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_blocked_async(void)
{
  return atomic_load_explicit(&num_samples_blocked_async, memory_order_relaxed);
}



//-----------------------------
// samples blocked dlopen
//-----------------------------

void
hpcrun_stats_num_samples_blocked_dlopen_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_blocked_dlopen, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_blocked_dlopen(void)
{
  return atomic_load_explicit(&num_samples_blocked_dlopen, memory_order_relaxed);
}



//-----------------------------
// cpu samples dropped
//-----------------------------

void
hpcrun_stats_num_samples_dropped_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_dropped, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_dropped(void)
{
  return atomic_load_explicit(&num_samples_dropped, memory_order_relaxed);
}


//-----------------------------
// acc samples recorded
//-----------------------------

void
hpcrun_stats_acc_samples_add(long value)
{
  atomic_fetch_add_explicit(&acc_samples, value, memory_order_relaxed);
}


long
hpcrun_stats_acc_samples(void)
{
  return atomic_load_explicit(&acc_samples, memory_order_relaxed);
}


//-----------------------------
// acc samples dropped
//-----------------------------

void
hpcrun_stats_acc_samples_dropped_add(long value)
{
  atomic_fetch_add_explicit(&acc_samples_dropped, value, memory_order_relaxed);
}


long
hpcrun_stats_acc_samples_dropped(void)
{
  return atomic_load_explicit(&acc_samples_dropped, memory_order_relaxed);
}


//-----------------------------
// acc trace records
//-----------------------------

void
hpcrun_stats_acc_trace_records_add(long value)
{
  atomic_fetch_add_explicit(&acc_trace_records, value, memory_order_relaxed);
}


long
hpcrun_stats_acc_trace_records(void)
{
  return atomic_load_explicit(&acc_trace_records, memory_order_relaxed);
}


//-----------------------------
// acc trace records dropped
//-----------------------------

void
hpcrun_stats_acc_trace_records_dropped_add(long value)
{
  atomic_fetch_add_explicit(&acc_trace_records_dropped, value, memory_order_relaxed);
}


long
hpcrun_stats_acc_trace_records_dropped(void)
{
  return atomic_load_explicit(&acc_trace_records_dropped, memory_order_relaxed);
}


//----------------------------
// partial unwinds
//----------------------------

void
hpcrun_stats_num_samples_partial_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_partial, 1L, memory_order_relaxed);
}

long
hpcrun_stats_num_samples_partial(void)
{
  return atomic_load_explicit(&num_samples_partial, memory_order_relaxed);
}

//-----------------------------
// samples segv
//-----------------------------

void
hpcrun_stats_num_samples_segv_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_segv, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_samples_segv(void)
{
  return atomic_load_explicit(&num_samples_segv, memory_order_relaxed);
}




//-----------------------------
// unwind intervals total
//-----------------------------

void
hpcrun_stats_num_unwind_intervals_total_inc(void)
{
  atomic_fetch_add_explicit(&num_unwind_intervals_total, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_unwind_intervals_total(void)
{
  return atomic_load_explicit(&num_unwind_intervals_total, memory_order_relaxed);
}



//-----------------------------
// unwind intervals suspicious
//-----------------------------

void
hpcrun_stats_num_unwind_intervals_suspicious_inc(void)
{
  atomic_fetch_add_explicit(&num_unwind_intervals_suspicious, 1L, memory_order_relaxed);
}


long
hpcrun_stats_num_unwind_intervals_suspicious(void)
{
  return atomic_load_explicit(&num_unwind_intervals_suspicious, memory_order_relaxed);
}

//------------------------------------------------------
// samples that include 1 or more successful troll steps
//------------------------------------------------------

void
hpcrun_stats_trolled_inc(void)
{
  atomic_fetch_add_explicit(&trolled, 1L, memory_order_relaxed);
}

long
hpcrun_stats_trolled(void)
{
  return atomic_load_explicit(&trolled, memory_order_relaxed);
}

//------------------------------------------------------
// total number of (unwind) frames in sample set
//------------------------------------------------------

void
hpcrun_stats_frames_total_inc(long amt)
{
  atomic_fetch_add_explicit(&frames_total, amt, memory_order_relaxed);
}

long
hpcrun_stats_frames_total(void)
{
  return atomic_load_explicit(&frames_total, memory_order_relaxed);
}
//-------------------------------------------------------
// number of (unwind) frames where libunwind failed
//------------------------------------------------------

void
hpcrun_stats_frames_libfail_total_inc(long amt)
{
  atomic_fetch_add_explicit(&frames_libfail_total, amt, memory_order_relaxed);
}

long
hpcrun_stats_frames_libfail_total(void)
{
  return atomic_load_explicit(&frames_libfail_total, memory_order_relaxed);
}

//---------------------------------------------------------------------
// total number of (unwind) frames in sample set that employed trolling
//---------------------------------------------------------------------

void
hpcrun_stats_trolled_frames_inc(long amt)
{
  atomic_fetch_add_explicit(&trolled_frames, amt, memory_order_relaxed);
}

long
hpcrun_stats_trolled_frames(void)
{
  return atomic_load_explicit(&trolled_frames, memory_order_relaxed);
}

//----------------------------
// samples yielded due to deadlock prevention
//----------------------------

void
hpcrun_stats_num_samples_yielded_inc(void)
{
  atomic_fetch_add_explicit(&num_samples_yielded, 1L, memory_order_relaxed);
}

long
hpcrun_stats_num_samples_yielded(void)
{
  return atomic_load_explicit(&num_samples_yielded, memory_order_relaxed);
}

//-----------------------------
// print summary
//-----------------------------

void
hpcrun_stats_print_summary(void)
{
  long cpu_blocked_async  = atomic_load_explicit(&num_samples_blocked_async, memory_order_relaxed);
  long cpu_blocked_dlopen = atomic_load_explicit(&num_samples_blocked_dlopen, memory_order_relaxed);
  long cpu_blocked = cpu_blocked_async + cpu_blocked_dlopen;

  long cpu_dropped = atomic_load_explicit(&num_samples_dropped, memory_order_relaxed);
  long cpu_segv = atomic_load_explicit(&num_samples_segv, memory_order_relaxed);
  long cpu_valid = atomic_load_explicit(&num_samples_attempted, memory_order_relaxed);
  long cpu_yielded = atomic_load_explicit(&num_samples_yielded, memory_order_relaxed);
  long cpu_total = atomic_load_explicit(&num_samples_total, memory_order_relaxed);

  long cpu_trolled = atomic_load_explicit(&trolled, memory_order_relaxed);

  long cpu_frames = atomic_load_explicit(&frames_total, memory_order_relaxed);
  long cpu_frames_trolled = atomic_load_explicit(&trolled_frames, memory_order_relaxed);
  long cpu_frames_libfail_total = atomic_load_explicit(&frames_libfail_total, memory_order_relaxed);

  long cpu_intervals_total = atomic_load_explicit(&num_unwind_intervals_total, memory_order_relaxed);
  long cpu_intervals_susp = atomic_load_explicit(&num_unwind_intervals_suspicious, memory_order_relaxed);

  long acc_samp = atomic_load_explicit(&acc_samples, memory_order_relaxed);
  long acc_samp_dropped = atomic_load_explicit(&acc_samples_dropped, memory_order_relaxed);

  long acc_trace = atomic_load_explicit(&acc_trace_records, memory_order_relaxed);
  long acc_trace_dropped = atomic_load_explicit(&acc_trace_records_dropped, memory_order_relaxed);

  hpcrun_memory_summary();

  AMSG("UNWIND ANOMALIES: total: %ld errant: %ld, total-frames: %ld, total-libunwind-fails: %ld",
       cpu_total, cpu_dropped, cpu_frames, cpu_frames_libfail_total );

  AMSG("ACC SUMMARY:\n"
       "         accelerator trace records: %ld (processed: %ld, dropped: %ld)\n"
       "         accelerator samples: %ld (recorded: %ld, dropped: %ld)",
       acc_trace + acc_trace_dropped, acc_trace, acc_trace_dropped,
       acc_samp + acc_samp_dropped, acc_samp, acc_samp_dropped
       );

  AMSG("SAMPLE ANOMALIES: blocks: %ld (async: %ld, dlopen: %ld), "
       "errors: %ld (segv: %ld, soft: %ld)",
       cpu_blocked, cpu_blocked_async, cpu_blocked_dlopen,
       cpu_dropped, cpu_segv, cpu_dropped - cpu_segv);

  AMSG("SUMMARY: samples: %ld (recorded: %ld, blocked: %ld, errant: %ld, trolled: %ld, yielded: %ld),\n"
       "         frames: %ld (trolled: %ld)\n"
       "         intervals: %ld (suspicious: %ld)",
       cpu_total, cpu_valid, cpu_blocked, cpu_dropped, cpu_trolled, cpu_yielded,
       cpu_frames, cpu_frames_trolled,
       cpu_intervals_total, cpu_intervals_susp
       );

  if (hpcrun_get_disabled()) {
    AMSG("SAMPLING HAS BEEN DISABLED");
  }

  // logs, retentions || adj.: recorded, retained, written

  if (ENABLED(UNW_VALID)) {
    hpcrun_validation_summary();
  }
}
