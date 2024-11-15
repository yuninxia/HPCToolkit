// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#define _GNU_SOURCE

#include <stdint.h>
#include <stdbool.h>
#include <ucontext.h>

#include "cct/cct_bundle.h"
#include "cct/cct.h"
#include "messages/messages.h"
#include "metrics.h"
#include "unresolved.h"

#include "../common/lean/placeholders.h"
#include "thread_data.h"
#include "hpcrun_stats.h"
#include "trace.h"
#include "trampoline/common/trampoline.h"
#include "utilities/ip-normalized.h"
#include "frame.h"
#include "unwind/common/backtrace_info.h"
#include "unwind/common/fence_enum.h"
#include "ompt/ompt-defer.h"
#include "ompt/ompt-callstack.h"

#include "cct_insert_backtrace.h"
#include "cct_backtrace_finalize.h"
#include "unwind/common/backtrace.h"
#include "thread_data.h"
#include "utilities/ip-normalized.h"


//
// Misc externals (not in an include file)
//
extern bool hpcrun_inbounds_main(void* addr);

//
// local variable records the on/off state of
// special recursive compression:
//
static bool retain_recursion = false;


static hpcrun_kernel_callpath_t hpcrun_kernel_callpath;


void
hpcrun_kernel_callpath_register(hpcrun_kernel_callpath_t kcp)
{
        hpcrun_kernel_callpath = kcp;
}

static cct_node_t*
cct_insert_raw_backtrace(cct_node_t* cct,
                            frame_t* path_beg, frame_t* path_end)
{
  TMSG(BT_INSERT, "%s : start", __func__);
  if (!cct) return NULL; // nowhere to insert

  // FIXME: POGLEDAJ KOLIKO ON PUTA KROZ OVO PRODJE

  ip_normalized_t parent_routine = {0, 0};
  for(; path_beg >= path_end; path_beg--){
    if ( (! retain_recursion) &&
         (path_beg >= path_end + 1) &&
         ip_normalized_eq(&(path_beg->the_function), &(parent_routine)) &&
         ip_normalized_eq(&(path_beg->the_function), &((path_beg-1)->the_function))) {
      TMSG(REC_COMPRESS, "recursive routine compression!");
    }
    else {
      cct_addr_t tmp = (cct_addr_t) {.ip_norm = path_beg->ip_norm};
      TMSG(BT_INSERT, "inserting addr (%d, %p)", tmp.ip_norm.lm_id, tmp.ip_norm.lm_ip);
      cct = hpcrun_cct_insert_addr(cct, &tmp, true);
    }
    parent_routine = path_beg->the_function;
  }
  hpcrun_cct_terminate_path(cct);
  // FIXME: vi3 consider this function
  return cct;
}


static cct_node_t*
help_hpcrun_backtrace2cct(cct_bundle_t* cct, ucontext_t* context,
        int metricId, hpcrun_metricVal_t metricIncr,
        int skipInner, int isSync, void *data);

//
//
void
hpcrun_set_retain_recursion_mode(bool mode)
{
  TMSG(REC_COMPRESS, "retain_recursion set to %s", mode ? "true" : "false");
  retain_recursion = mode;
}

bool
hpcrun_get_retain_recursion_mode()
{
  return retain_recursion;
}

// See usage in header.
cct_node_t*
hpcrun_cct_insert_backtrace(cct_node_t* treenode, frame_t* path_beg, frame_t* path_end)
{
  TMSG(FENCE, "insert backtrace into treenode %p", treenode);
  TMSG(FENCE, "backtrace below");
  bool bt_ins = ENABLED(BT_INSERT);
  if (ENABLED(FENCE)) {
    ENABLE(BT_INSERT);
  }

  cct_node_t* path = cct_insert_raw_backtrace(treenode, path_beg, path_end);
  if (! bt_ins) DISABLE(BT_INSERT);

  return path;
}

// See usage in header.
cct_node_t*
hpcrun_cct_insert_backtrace_w_metric(cct_node_t* treenode,
                                     int metric_id,
                                     frame_t* path_beg, frame_t* path_end,
                                     cct_metric_data_t datum, void *data_aux)
{
  cct_node_t* path = hpcrun_cct_insert_backtrace(treenode, path_beg, path_end);

  if (hpcrun_kernel_callpath) {
    path = hpcrun_kernel_callpath(path, data_aux);
  }

  metric_data_list_t* mset = hpcrun_reify_metric_set(path, metric_id);

  metric_upd_proc_t* upd_proc = hpcrun_get_metric_proc(metric_id);
  if (upd_proc) {
    upd_proc(metric_id, mset, datum);
  }

  // POST-INVARIANT: metric set has been allocated for 'path'

  return path;
}

//
// Insert new backtrace in cct
//

cct_node_t*
hpcrun_cct_insert_bt(cct_node_t* node,
                     int metricId,
                     backtrace_t* bt,
                     cct_metric_data_t datum)
{
  return hpcrun_cct_insert_backtrace_w_metric(node, metricId,
                                              bt->beg + bt->len - 1,
                                              bt->beg,
                                              datum, NULL);
}


//-----------------------------------------------------------------------------
// function: hpcrun_backtrace2cct
// purpose:
//     if successful, returns the leaf node representing the sample;
//     otherwise, returns NULL.
//-----------------------------------------------------------------------------

//
// TODO: one more flag:
//   backtrace needs to either:
//       IGNORE_TRAMPOLINE (usually, but not always called when isSync is true)
//       PLACE_TRAMPOLINE  (standard for normal async samples).
//

cct_node_t*
hpcrun_backtrace2cct(cct_bundle_t* cct, ucontext_t* context,
                     int metricId, hpcrun_metricVal_t metricIncr,
                     int skipInner, int isSync, void *data)
{
  cct_node_t* n = NULL;
  TMSG(BT_INSERT,"regular (NON-lush) backtrace2cct invoked");
  n = help_hpcrun_backtrace2cct(cct, context,
                                metricId, metricIncr,
                                skipInner, isSync, data);

  // N.B.: for lush_backtrace() it may be that n = NULL

  return n;
}


cct_node_t*
hpcrun_cct_record_backtrace(
  cct_bundle_t* cct,
  bool partial,
  backtrace_info_t *bt,
  bool tramp_found
)
{
  TMSG(FENCE, "Recording backtrace");
  thread_data_t* td = hpcrun_get_thread_data();
  cct_node_t* cct_cursor = cct->tree_root;
  TMSG(FENCE, "Initially picking tree root = %p", cct_cursor);
  if (tramp_found) {
    // start insertion below caller's frame, which is marked with the trampoline
    cct_cursor = hpcrun_cct_parent(td->tramp_cct_node);
    TMSG(FENCE, "Tramp found ==> cursor = %p", cct_cursor);
  }
  if (partial) {
    cct_cursor = cct->partial_unw_root;
    TMSG(FENCE, "Partial unwind ==> cursor = %p", cct_cursor);
  }
  if (bt->fence == FENCE_THREAD) {
    cct_cursor = cct->thread_root;
    TMSG(FENCE, "Thread stop ==> cursor = %p", cct_cursor);
  }

  TMSG(FENCE, "sanity check cursor = %p", cct_cursor);
  TMSG(FENCE, "further sanity check: bt->last frame = (%d, %p)",
       bt->last->ip_norm.lm_id, bt->last->ip_norm.lm_ip);

  return hpcrun_cct_insert_backtrace(cct_cursor, bt->last, bt->begin);

}

cct_node_t*
hpcrun_cct_record_backtrace_w_metric(cct_bundle_t* cct, bool partial,
                                     backtrace_info_t *bt, bool tramp_found,
                                     int metricId, hpcrun_metricVal_t metricIncr,
                                     void *data)
{
  TMSG(FENCE, "Recording backtrace");
  TMSG(BT_INSERT, "Record backtrace w metric to id %d, incr = %d",
       metricId, metricIncr.i);

  thread_data_t* td = hpcrun_get_thread_data();
  cct_node_t* cct_cursor = cct->tree_root;
  TMSG(FENCE, "Initially picking tree root = %p", cct_cursor);
  if (tramp_found) {
    // start insertion below caller's frame, which is marked with the trampoline
    cct_cursor = hpcrun_cct_parent(td->tramp_cct_node);
    TMSG(FENCE, "Tramp found ==> cursor = %p", cct_cursor);
  }
  if (partial) {
    cct_cursor = cct->partial_unw_root;
    TMSG(FENCE, "Partial unwind ==> cursor = %p", cct_cursor);
  }
  if (bt->fence == FENCE_THREAD) {
    cct_cursor = cct->thread_root;
    TMSG(FENCE, "Thread stop ==> cursor = %p", cct_cursor);
  }

  cct_cursor = cct_cursor_finalize(cct, bt, cct_cursor);

  TMSG(FENCE, "sanity check cursor = %p", cct_cursor);
  TMSG(FENCE, "further sanity check: bt->last frame = (%d, %p)",
       bt->last->ip_norm.lm_id, bt->last->ip_norm.lm_ip);

  return hpcrun_cct_insert_backtrace_w_metric(cct_cursor, metricId,
                                              bt->last, bt->begin,
                                              (cct_metric_data_t) metricIncr, data);

}


static cct_node_t*
help_hpcrun_backtrace2cct(cct_bundle_t* bundle, ucontext_t* context,
                          int metricId,
                          hpcrun_metricVal_t metricIncr,
                          int skipInner, int isSync, void *data)
{
  bool tramp_found;

  thread_data_t* td = hpcrun_get_thread_data();
  backtrace_info_t bt;

  // initialize bt
  memset(&bt, 0, sizeof(bt));

  bool success = hpcrun_generate_backtrace(&bt, context, skipInner);

  if (!success != bt.partial_unwind)
    hpcrun_terminate();

  tramp_found = bt.has_tramp;

  //
  // Check to make sure node below monitor_main is "main" node
  //
  // TMSG(GENERIC1, "tmain chk");
  if (ENABLED(CHECK_MAIN)) {
    if ( bt.fence == FENCE_MAIN &&
         ! bt.partial_unwind &&
         ! tramp_found &&
         (bt.last == bt.begin ||
          ! hpcrun_inbounds_main(hpcrun_frame_get_unnorm(bt.last - 1)))) {
      hpcrun_bt_dump(TD_GET(btbuf_cur), "WRONG MAIN");
      hpcrun_stats_num_samples_dropped_inc();
      bt.partial_unwind = true;
    }
  }

  cct_backtrace_finalize(&bt, isSync);

  if (bt.partial_unwind) {
    if (ENABLED(NO_PARTIAL_UNW)){
      return NULL;
    }

    TMSG(PARTIAL_UNW, "recording partial unwind from graceful failure, "
         "len partial unw = %d", (bt.last - bt.begin)+1);
    hpcrun_stats_num_samples_partial_inc();
  }

  cct_node_t* n =
    hpcrun_cct_record_backtrace_w_metric(bundle, bt.partial_unwind, &bt,
                                         tramp_found,
                                         metricId, metricIncr, data);

  if (!ompt_eager_context_p()) {
    // FIXME vi3: a big hack
    if (isSync == 33) {
      provide_callpath_for_end_of_the_region(&bt, n);
    } else {
      provide_callpath_for_regions_if_needed(&bt, n);
    }
  }

  if (bt.n_trolls != 0) hpcrun_stats_trolled_inc();
  hpcrun_stats_frames_total_inc((long)(bt.last - bt.begin + 1));
  hpcrun_stats_trolled_frames_inc((long) bt.n_trolls);

  if (ENABLED(USE_TRAMP)){
    TMSG(TRAMP, "--NEW SAMPLE--: Remove old trampoline");
    hpcrun_trampoline_remove();
    if (!bt.partial_unwind) {
      td->tramp_frame = td->cached_bt_frame_beg;
      td->prev_dLCA = td->dLCA;
      td->dLCA = 0;
      TMSG(TRAMP, "--NEW SAMPLE--: Insert new trampoline");
      hpcrun_trampoline_insert(n);
    } else
      td->prev_dLCA = HPCTRACE_FMT_DLCA_NULL;
  }

  return n;
}
