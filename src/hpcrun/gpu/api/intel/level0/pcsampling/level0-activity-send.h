#ifndef LEVEL0_ACTIVITY_SEND_H
#define LEVEL0_ACTIVITY_SEND_H

#include <deque>
#include <iostream>
#include <map>
#include <unordered_map>

#include "../../../../activity/gpu-activity.h"
#include "level0-kernel-properties.h"

extern "C" {
  #include "../../../../activity/gpu-activity-channel.h"
}

void
zeroSendActivities
(
  std::deque<gpu_activity_t*>& activities
);

void
zeroLogActivities
(
  const std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, KernelProperties>& kprops
);

#endif // LEVEL0_ACTIVITY_SEND_H