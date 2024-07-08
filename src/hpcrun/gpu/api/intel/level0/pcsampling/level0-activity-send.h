#ifndef LEVEL0_ACTIVITY_SEND_H
#define LEVEL0_ACTIVITY_SEND_H

#include <deque>
#include <map>
#include "../../../../activity/gpu-activity.h"
#include "../../../../activity/gpu-activity-channel.h"
#include "level0-kernel-properties.h"

void
SendActivities
(
  std::deque<gpu_activity_t*>& activities
);

void
LogActivities
(
  const std::deque<gpu_activity_t*>& activities, 
  const std::map<uint64_t, KernelProperties>& kprops
);

#endif // LEVEL0_ACTIVITY_SEND_H