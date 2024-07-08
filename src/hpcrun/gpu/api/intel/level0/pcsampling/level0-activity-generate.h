#ifndef LEVEL0_ACTIVITY_GENERATE_H
#define LEVEL0_ACTIVITY_GENERATE_H

#include <deque>
#include <map>
#include <unordered_map>
#include "../../../../activity/gpu-activity.h"
#include "level0-metric.h"
#include "level0-kernel-properties.h"

std::deque<gpu_activity_t*> 
GenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, l0_metric::EuStalls>& eustalls,
  uint64_t& correlation_id
);

#endif // LEVEL0_ACTIVITY_GENERATE_H