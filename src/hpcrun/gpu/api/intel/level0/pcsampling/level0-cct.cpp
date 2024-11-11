// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-cct.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

cct_node_t*
zeroGetCctNode
(
  uint64_t correlation_id
)
{
  auto it = cid_cct_node_.find(correlation_id);
  if (it == cid_cct_node_.end()) {
    return nullptr;
  }

  return it->second;
}

void
zeroLogcCidMap
(
  void
)
{
  std::cout << "\n=== Debug: cid_cct_node_ Map Content ===" << std::endl;
  
  for (const auto& [cid, node] : cid_cct_node_) {
    cct_node_t* host_node = zeroGetCctNode(cid);
    ip_normalized_t host_ip = hpcrun_cct_addr(host_node)->ip_norm;
    std::cout << "CID: 0x" << std::hex << cid << std::dec
              << " -> Node: " << node
              << " -> Host IP: 0x" << std::hex << host_ip.lm_ip
              << std::endl;
  }
  std::cout << "Total mappings: " << cid_cct_node_.size() << std::endl;
  std::cout << "=====================================\n" << std::endl;
}


void
zeroInitIdTuple
(
  thread_data_t* td,
  uint32_t device_id,
  int thread_id
) 
{
  pms_id_t ids[IDTUPLE_MAXTYPES];
  id_tuple_t id_tuple;
  id_tuple_constructor(&id_tuple, ids, IDTUPLE_MAXTYPES);

  id_tuple_push_back(&id_tuple, IDTUPLE_COMPOSE(IDTUPLE_NODE, IDTUPLE_IDS_LOGIC_LOCAL), OSUtil_hostid(), 0);
  
  id_tuple_push_back(&id_tuple, IDTUPLE_COMPOSE(IDTUPLE_GPUDEVICE, IDTUPLE_IDS_LOGIC_ONLY), device_id, device_id);
  
  id_tuple_push_back(&id_tuple, IDTUPLE_COMPOSE(IDTUPLE_THREAD, IDTUPLE_IDS_LOGIC_ONLY), thread_id, thread_id);
  
  id_tuple_copy(&td->core_profile_trace_data.id_tuple, &id_tuple, hpcrun_malloc);
}

thread_data_t* 
zeroInitThreadData
(
  int thread_id,
  bool demand_new_thread
)
{
  hpcrun_thread_init_mem_pool_once(thread_id, nullptr, HPCRUN_NO_TRACE, demand_new_thread);

  thread_data_t* td = hpcrun_get_thread_data();
  if (!td) {
    std::cerr << "Failed to initialize thread data for thread " << thread_id << std::endl;
    return nullptr;
  }

  return td;
}

bool
zeroBuildCCT
(
  thread_data_t* td, 
  std::deque<gpu_activity_t*>& activities
)
{
  zeroLogcCidMap();

  cct_node_t* root = td->core_profile_trace_data.epoch->csdata.tree_root;
  if (!root) return false;

  ip_normalized_t gpu_op_ip = gpu_op_placeholder_ip(gpu_placeholder_type_kernel);
  cct_node_t* gpu_op_node = hpcrun_cct_insert_ip_norm(root, gpu_op_ip, true);
  if (!gpu_op_node) return false;

  for (auto activity : activities) {
    uint64_t correlation_id = activity->details.pc_sampling.correlation_id;
    
    cct_node_t* host_node = zeroGetCctNode(correlation_id);
    if (!host_node) continue;

    // cct_node_t* parent_node = hpcrun_cct_parent(host_node);
    // if (!parent_node) continue;
    // std::cout << "host_node: " << host_node << ", persistent_id: " << hpcrun_cct_persistent_id(host_node)
    //           << ", parent_node: " << parent_node << ", persistent_id: " << hpcrun_cct_persistent_id(parent_node)
    //           << std::endl;

    ip_normalized_t host_ip = hpcrun_cct_addr(host_node)->ip_norm;
    cct_node_t* new_host_node = hpcrun_cct_insert_ip_norm(gpu_op_node, host_ip, true);
    if (!new_host_node) {
      continue;
    }

    ip_normalized_t act_ip = activity->details.pc_sampling.pc;
    cct_node_t* activity_node = hpcrun_cct_insert_ip_norm(new_host_node, act_ip, true);
    if (!activity_node) {
      continue;
    }

    activity->cct_node = activity_node;

    gpu_metrics_attribute(activity);

    delete activity;
  }

  activities.clear();

  hpcrun_write_profile_data(&td->core_profile_trace_data);

  return true;
}
