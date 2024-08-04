#include "level0-activity-generate.h"

void
zeroGenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, EuStalls>& eustalls,
  uint64_t& correlation_id,
  std::deque<gpu_activity_t*>& activities,
  ze_kernel_handle_t running_kernel
)
{
  if (running_kernel == nullptr) {
    return;
  }

  activities.clear();
  std::unordered_map<std::string, uint64_t> kernel_cids;
  for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
    if (kernel_cids.find(rit->second.name) == kernel_cids.end()) {
      kernel_cids[rit->second.name] = correlation_id;
    }
  }

  size_t name_len = 0;
  ze_result_t status = zeKernelGetName(running_kernel, &name_len, nullptr);
  std::vector<char> kernel_name(name_len);
  if (status == ZE_RESULT_SUCCESS && name_len > 0) {
    status = zeKernelGetName(running_kernel, &name_len, kernel_name.data());
  }
  std::string running_kernel_name = name_len > 0 ? std::string(kernel_name.begin(), kernel_name.end() - 1) : "UnknownKernel";

#if 0
  std::cout << "Running kernel handle: " << running_kernel << " Name: " << running_kernel_name << std::endl;
#endif

  for (auto it = eustalls.begin(); it != eustalls.end(); ++it) {
    for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
      if ((rit->first <= it->first) && ((it->first - rit->first) < rit->second.size)) {
        uint64_t cid = kernel_cids[rit->second.name];
        zeroActivityTranslate(activities, it, rit, cid);
        break;
      }
    }
  }
}