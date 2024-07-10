#include "level0-activity-send.h"

void
zeroSendActivities
(
  std::deque<gpu_activity_t*>& activities
) 
{
  for (auto activity : activities) {
    uint32_t thread_id = gpu_activity_channel_correlation_id_get_thread_id(activity->details.instruction.correlation_id);
    gpu_activity_channel_t *channel = gpu_activity_channel_lookup(thread_id);
    gpu_activity_channel_send(channel, activity);
  }
}

void 
zeroLogActivities
(
  const std::deque<gpu_activity_t*>& activities,
  const std::map<uint64_t, KernelProperties>& kprops
)
{
  std::map<uint64_t, std::pair<std::string, uint64_t>> kernel_info;
  for (const auto& [base, prop] : kprops) {
    uint64_t adjusted_base = base + 0x800000000000;
    kernel_info[adjusted_base] = {prop.name, adjusted_base};
  }

  std::cout << std::endl;

  std::unordered_map<uint64_t, int> cid_count;

  for (const auto* activity : activities) {
    uint64_t instruction_pc_lm_ip = activity->details.instruction.pc.lm_ip;
    uint64_t cid = activity->details.pc_sampling.correlation_id;
    uint16_t lm_id = activity->details.pc_sampling.pc.lm_id;
    
    auto it = kernel_info.upper_bound(instruction_pc_lm_ip);
    if (it != kernel_info.begin()) {
      --it;
    }
    const auto& [kernel_name, kernel_base] = (it != kernel_info.end()) ? it->second : std::pair<std::string, uint64_t>("Unknown", 0);

    uint64_t offset = (kernel_base != 0) ? (instruction_pc_lm_ip - kernel_base) : 0;

    std::cerr << "PC Sample" << std::endl;
    std::cerr << "PC sampling: sample(pc=0x" << std::hex << activity->details.pc_sampling.pc.lm_ip
              << ", cid=" << cid
              << ", kernel_name=" << kernel_name
              << ")" << std::endl;
    std::cerr << "PC sampling: normalize 0x" << std::hex << instruction_pc_lm_ip
              << " --> [" << std::dec << lm_id << ", 0x" 
              << std::hex << offset << "]" << std::endl;

    cid_count[cid]++;
  }

  std::cout << std::dec << std::endl;
  std::cout << "Correlation ID Statistics:" << std::endl;
  for (const auto& [cid, count] : cid_count) {
    std::cout << "Correlation ID: " << cid << " Count: " << count << std::endl;
  }
}
