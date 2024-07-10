#ifndef LEVEL0_ACTIVITY_GENERATE_H
#define LEVEL0_ACTIVITY_GENERATE_H

#include <deque>
#include <map>
#include <unordered_map>

#include "../../../../activity/gpu-activity.h"
#include "level0-activity-translate.h"
#include "level0-kernel-properties.h"
#include "level0-metric.h"

void
zeroGenerateActivities
(
  const std::map<uint64_t, KernelProperties>& kprops, 
  std::map<uint64_t, EuStalls>& eustalls,
  uint64_t& correlation_id,
  std::deque<gpu_activity_t*>& activities
);

#endif // LEVEL0_ACTIVITY_GENERATE_H