#include "level0-activity-generate.h"
#include "level0-activity-translate.h"

std::deque<gpu_activity_t*>
GenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, l0_metric::EuStalls>& eustalls,
  uint64_t& correlation_id
)
{
  std::deque<gpu_activity_t*> activities;
  std::unordered_map<std::string, uint64_t> kernel_cids;
  for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
    if (kernel_cids.find(rit->second.name) == kernel_cids.end()) {
      kernel_cids[rit->second.name] = correlation_id;
    }
  }

  for (auto it = eustalls.begin(); it != eustalls.end(); ++it) {
    for (auto rit = kprops.crbegin(); rit != kprops.crend(); ++rit) {
      if ((rit->first <= it->first) && ((it->first - rit->first) < rit->second.size)) {
        uint64_t cid = kernel_cids[rit->second.name];
        level0_activity_translate(activities, it, rit, cid);
        break;
      }
    }
  }

  return activities;
}
