// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../decl-init-cast.h"
#include "../../../gpu/activity/gpu-op-placeholders.h"
#include "../../../libmonitor/monitor.h"
#include "../../../thread_data.h"
#include "gpu-cid-map.h"
#include "gpu-cid-map.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// generic code for splay tree
//******************************************************************************

typedef struct gpu_cid_map_entry_t {
    SPLAY_TREE_ENTRY_DATA(struct gpu_cid_map_entry_t);
    uint64_t correlation_id;
    gpu_cid_map_info_t pair;
} gpu_cid_map_entry_t;


#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       uint64_t
#define SPLAY_TREE_KEY_FIELD      correlation_id
#define SPLAY_TREE_ENTRY_TYPE     gpu_cid_map_entry_t

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"



//******************************************************************************
// private data
//******************************************************************************

static __thread st_t cid_map;



//*****************************************************************************
// private operations
//*****************************************************************************

static void
insert_gpu_runtime_root
(
  void
)
{
  thread_data_t* td = hpcrun_get_thread_data();
  cct_bundle_t* cct = &(td->core_profile_trace_data.epoch->csdata);

  cct_node_t *rt_ph = hpcrun_cct_insert_ip_norm(cct->top,
    gpu_op_placeholder_ip(gpu_placeholder_type_runtime), false);

  gpu_cid_map_insert(GPU_RUNTIME_PH_CID, rt_ph, ip_normalized_NULL);

  gpu_cid_map_insert(PARTIAL_UNWIND_PH_CID, cct->partial_unw_root, ip_normalized_NULL);
}


static void
gpu_cid_map_ensure_init
(
  void
)
{
  static __thread bool do_init = true;
  if (!do_init) return;

  do_init = false;
  cid_map = (st_t) SPLAY_TREE_INITIALIZER;
  insert_gpu_runtime_root();
}



//*****************************************************************************
// interface operations
//*****************************************************************************

bool
gpu_cid_map_insert
(
  uint64_t correlation_id,
  cct_node_t *node,
  ip_normalized_t kernel_ip
)
{
  gpu_cid_map_ensure_init();

  DECL_INIT_CAST(gpu_cid_map_entry_t *, entry, malloc(sizeof(gpu_cid_map_entry_t)));

  *entry = (gpu_cid_map_entry_t) {
    .correlation_id = correlation_id,
    .pair.node = node,
    .pair.kernel_ip = kernel_ip
  };

  bool inserted = st_insert(&cid_map, entry);
  assert(inserted);

  PRINT("gpu_cid_map_insert(cid=0x%lx, cct_node=%p) on thread %d \n", correlation_id, node, monitor_get_thread_num());

  return inserted;
}


gpu_cid_map_info_t *
gpu_cid_map_find
(
  uint64_t correlation_id
)
{
  gpu_cid_map_ensure_init();

  gpu_cid_map_entry_t *entry =
    st_lookup(&cid_map, correlation_id);

  PRINT("gpu_cid_map_find(cid=0x%lx) on thread %d returns pair=(node=%p,kernel_ip=(%d,%lx))\n",
    correlation_id, monitor_get_thread_num(),
    entry ? entry->pair.node : 0,
    entry ? entry->pair.kernel_ip.lm_id : 0,
    entry ? entry->pair.kernel_ip.lm_ip : 0);

  assert(entry);

  return &entry->pair;
}


bool
gpu_cid_map_delete
(
  uint64_t correlation_id
)
{
  gpu_cid_map_ensure_init();

  gpu_cid_map_entry_t *entry =
    st_delete(&cid_map, correlation_id);

  assert(entry);

  PRINT("gpu_cid_map_delete(cid=0x%lx)\n)", correlation_id);

  return entry != NULL;
}
