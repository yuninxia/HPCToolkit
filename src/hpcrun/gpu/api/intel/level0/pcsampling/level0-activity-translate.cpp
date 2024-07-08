#include "level0-activity-translate.h"
#include "../level0-id-map.h"

gpu_inst_stall_t
level0_convert_stall_reason
(
  const l0_metric::EuStalls& stall
) 
{
  gpu_inst_stall_t stall_reason = GPU_INST_STALL_NONE;
  uint64_t max_value = 0;

  if (stall.control_ > max_value) {
    max_value = stall.control_;
    stall_reason = GPU_INST_STALL_OTHER; // TBD
  }
  if (stall.pipe_ > max_value) {
    max_value = stall.pipe_;
    stall_reason = GPU_INST_STALL_PIPE_BUSY;
  }
  if (stall.send_ > max_value) {
    max_value = stall.send_;
    stall_reason = GPU_INST_STALL_GMEM; // TBD
  }
  if (stall.dist_ > max_value) {
    max_value = stall.dist_;
    stall_reason = GPU_INST_STALL_PIPE_BUSY; // TBD
  }
  if (stall.sbid_ > max_value) {
    max_value = stall.sbid_;
    stall_reason = GPU_INST_STALL_IDEPEND; // TBD
  }
  if (stall.sync_ > max_value) {
    max_value = stall.sync_;
    stall_reason = GPU_INST_STALL_SYNC;
  }
  if (stall.insfetch_ > max_value) {
    max_value = stall.insfetch_;
    stall_reason = GPU_INST_STALL_IFETCH;
  }
  if (stall.other_ > max_value) {
    max_value = stall.other_;
    stall_reason = GPU_INST_STALL_OTHER;
  }

  return stall_reason;
}

bool 
level0_convert_pcsampling
(
  gpu_activity_t* activity, 
  std::map<uint64_t, l0_metric::EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit,
  uint64_t correlation_id
)
{
  activity->kind = GPU_ACTIVITY_PC_SAMPLING;

  KernelProperties kernel_props = rit->second;
  l0_metric::EuStalls stall = it->second;

  uint32_t module_id_uint32 = hex_string_to_uint<uint32_t>(kernel_props.module_id.substr(0, 8));
  zebin_id_map_entry_t *entry = zebin_id_map_lookup(module_id_uint32);
  if (entry != nullptr) {
    uint32_t hpctoolkit_module_id = zebin_id_map_entry_hpctoolkit_id_get(entry);
    activity->details.pc_sampling.pc.lm_id = (uint16_t)hpctoolkit_module_id;
  }

  activity->details.pc_sampling.pc.lm_ip = it->first + 0x800000000000; // real = it->first; base = rit->first; offset = real - base;
  activity->details.pc_sampling.correlation_id = correlation_id;
  activity->details.pc_sampling.samples = stall.active_ + stall.control_ + stall.pipe_ + stall.send_ + stall.dist_ + stall.sbid_ + stall.sync_ + stall.insfetch_ + stall.other_;
  activity->details.pc_sampling.latencySamples = activity->details.pc_sampling.samples - stall.active_;
  activity->details.pc_sampling.stallReason = level0_convert_stall_reason(stall);

  return true;
}

void 
level0_activity_translate
(
  std::deque<gpu_activity_t*>& activities, 
  std::map<uint64_t, l0_metric::EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit,
  uint64_t correlation_id
) 
{
  gpu_activity_t* activity = new gpu_activity_t();
  gpu_activity_init(activity);

  if (level0_convert_pcsampling(activity, it, rit, correlation_id)) {
    activities.push_back(activity);
  } else {
    delete activity;
  }
}