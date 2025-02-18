// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef THREAD_DATA_H
#define THREAD_DATA_H

// This is called "thread data", but it applies to process as well
// (there is just 1 thread).

#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "trace.h"
#include "sample_sources_registered.h"
#include "memory/newmem.h"
#include "epoch.h"
#include "cct2metrics.h"
#include "core_profile_trace_data.h"
#include "ompt/omp-tools.h"
#include "logical/common.h"

#include "unwind/common/backtrace.h"
#include "unwind/common/uw_hash.h"

#include "../common/lean/hpcio.h"
#include "../common/lean/hpcio-buffer.h"

#define TOOL_THREAD_ID (-1)

typedef struct {
  sigjmp_buf jb;
} sigjmp_buf_t;

typedef struct gpu_data_t {
  // True if this thread is at CuXXXXSynchronize.
  bool is_thread_at_cuda_sync;
  // maintains state to account for overload potential
  uint8_t overload_state;
  // current active stream
  uint64_t active_stream;
  // last examined event
  void * event_node;
  // maintain the total number of threads (global: think shared
  // blaming) at synchronize (could be device/stream/...)
  uint64_t accum_num_sync_threads;
        // holds the number of times the above accum_num_sync_threads
        // is updated
  uint64_t accum_num_samples;
} gpu_data_t;



/* ******
   TODO:

   thread_data_t needs more structure.
   Organize according to following plan:

   backtrace_buffer
     factor out of the current epoch, and put it as a separate item in thread_data
     it will become an opaque datatype
   each epoch has:
       loadmap   (currently called epoch)
       cct       (the main sample data container)
       cct_ctxt  (pthread creation context)
    exceptions package (items used when something goes wrong)
        bad_unwind
        mem_error
        handling_sample (used to distinguish a sample-based segv from a user segv)
    thread_locks package
        (each of the 'lock' elements is true if this thread owns the lock.
         locks must be released in an exceptional situation)
        fnbounds_lock
        splay_lock
    sample source package
       event_set
       ss_state
    trampoline
       all trampoline-related items should be collected into a struct
       NOTE: a single backtrace_buffer item will replace the collection of fields
             that currently implement the cached backtrace.
    io_support
       This is a collection of files:
         trace_file
         hpcrun_file

    debug
       a few bools & integers. General purpose. Used for simulating error conditions.
 */


typedef struct thread_data_t {
  // ----------------------------------------
  // support for blame shifting
  // ----------------------------------------
  int idle;              // indicate whether the thread is idle
  uint64_t blame_target; // a target for directed blame

  int last_synch_sample;
  int last_sample;

  int overhead; // indicate whether the thread is overhead

  int lockwait; // if thread is in work & lockwait state, it is waiting for a lock
  void *lockid; // pointer pointing to the lock

  uint64_t region_id; // record the parallel region the thread is currently in

  uint64_t outer_region_id; // record the outer-most region id the thread is currently in
  cct_node_t* outer_region_context; // cct node associated with the outermost region

  int defer_flag; //whether should defer the context creation

  cct_node_t *omp_task_context; // pass task context from elider to cct insertion
  int master; // whether the thread is the master thread
  int team_master; // whether the thread is the team master thread

  int defer_write; // whether should defer the write

  int reuse; // mark whether the td is ready for reuse

  int add_to_pool;

  int omp_thread;
  uint64_t last_bar_time_us;

  // ----------------------------------------
  // hpcrun_malloc() memory data structures
  // ----------------------------------------
  hpcrun_meminfo_t memstore;
  int              mem_low;

  // ----------------------------------------
  // sample sources
  // ----------------------------------------

  source_state_t* ss_state; // allocate at initialization time
  source_info_t*  ss_info;  // allocate at initialization time

  struct sigevent sigev;   // POSIX real-time timer
  timer_t        timerid;
  bool           timer_init;

  uint64_t       last_time_us; // microseconds

  // ----------------------------------------
  // core_profile_trace_data contains the following
  // epoch: loadmap + cct + cct_ctxt
  // cct2metrics map: associate a metric_set with
  // tracing: trace_min_time_us and trace_max_time_us
  // IO support file handle: hpcrun_file;
  // Perf event support
  // ----------------------------------------

  core_profile_trace_data_t core_profile_trace_data;

  // ----------------------------------------
  // backtrace buffer
  // ----------------------------------------

  // btbuf_beg                                                  btbuf_end
  // |                                                            |
  // v low VMAs                                                   v
  // +------------------------------------------------------------+
  // [new backtrace         )              [cached backtrace      )
  // +------------------------------------------------------------+
  //                        ^              ^
  //                        |              |
  //                    btbuf_cur       btbuf_sav

  frame_t* btbuf_cur;  // current frame when actively constructing a backtrace
  frame_t* btbuf_beg;  // beginning of the backtrace buffer
                       // also, location of the innermost frame in
                       // newly recorded backtrace (although skipInner may
                       // adjust the portion of the backtrace that is recorded)
  frame_t* btbuf_end;  // end of the current backtrace buffer
  frame_t* btbuf_sav;  // innermost frame in cached backtrace


  backtrace_t bt;     // backtrace used for unwinding
  uw_hash_table_t *uw_hash_table;

  // ----------------------------------------
  // trampoline
  // ----------------------------------------
  bool    tramp_present;   // TRUE if a trampoline installed; FALSE otherwise
  void*   tramp_retn_addr; // return address that the trampoline replaced
  void*   tramp_loc;       // current (stack) location of the trampoline
  size_t  cached_frame_count; // (sanity check) length of cached frame list

  frame_t* cached_bt_buf_beg;  // the begin of the cached backtrace buffer
  frame_t* cached_bt_frame_beg;  // the begin of cached frames
  frame_t* cached_bt_buf_frame_end;  // the end of the cached backtrace buffer & end of cached frames

  frame_t* tramp_frame;       // (cached) frame assoc. w/ cur. trampoline loc.
  cct_node_t* tramp_cct_node; // cct node associated with the trampoline

  uint32_t prev_dLCA; // distance to LCA in the CCT for the previous sample
  uint32_t dLCA; // distance to LCA in the CCT

  // ----------------------------------------
  // exception stuff
  // ----------------------------------------
  sigjmp_buf_t     *current_jmp_buf;
  sigjmp_buf_t     bad_interval;
  sigjmp_buf_t     bad_unwind;

  bool             deadlock_drop;
  int              handling_sample;
  int              fnbounds_lock;

  // ----------------------------------------
  // Logical unwinding
  // ----------------------------------------

  /// Stack of active logical regions in this thread.
  logical_region_stack_t logical_regs;

  // ----------------------------------------
  // debug stuff
  // ----------------------------------------
  bool debug1;

  // ----------------------------------------
  // miscellaneous
  // ----------------------------------------
  // Set to 1 while inside hpcrun code for safe sampling.
  int inside_hpcrun;

  // True if this thread is inside dlopen or dlclose.  A synchronous
  // override that is called from dlopen (eg, malloc) must skip this
  // sample or else deadlock on the dlopen lock.
  bool inside_dlfcn;


#ifdef ENABLE_CUDA
  gpu_data_t gpu_data;
#endif

  // this flag is used to identify application thread
  // USE CASE: Certain metrics (like GPU_IDLE_CAUSE from blame-shifting) should only be
  // attributed to main thread (thread 0) and not background threads.
  bool application_thread_0;

  uint64_t gpu_trace_prev_time;

  uint64_t ga_idleness_count;

} thread_data_t;


static const size_t HPCRUN_TraceBufferSz = HPCIO_RWBufferSz;


void
hpcrun_init_pthread_key
(
  void
);


void
hpcrun_set_thread0_data
(
  void
);


void
hpcrun_set_thread_data
(
  thread_data_t *td
);


#define TD_GET(field) hpcrun_get_thread_data()->field

extern thread_data_t* (*hpcrun_get_thread_data)(void);
extern bool           (*hpcrun_td_avail)(void);
extern thread_data_t* hpcrun_safe_get_td(void);
extern thread_data_t* hpcrun_allocate_thread_data(int id);


void
hpcrun_unthreaded_data
(
  void
);


void
hpcrun_threaded_data
(
  void
);


void
hpcrun_thread_init_mem_pool_once
(
  int id,
  cct_ctxt_t *thr_ctxt,
  hpcrun_trace_type_t trace,
  bool demand_new_thread
);


void
hpcrun_thread_data_init
(
  int id,
  cct_ctxt_t* thr_ctxt,
  int is_child,
  size_t n_sources
);


void
hpcrun_cached_bt_adjust_size
(
  size_t n
);


frame_t*
hpcrun_expand_btbuf
(
  void
);


void
hpcrun_ensure_btbuf_avail
(
  void
);


void
hpcrun_thread_data_reuse_init
(
  cct_ctxt_t* thr_ctxt
);


void
hpcrun_cached_bt_adjust_size
(
  size_t n
);



void hpcrun_id_tuple_cputhread(thread_data_t *td);

// utilities to match previous api
#define hpcrun_get_thread_epoch()  TD_GET(core_profile_trace_data.epoch)

#endif // !defined(THREAD_DATA_H)
