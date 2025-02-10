// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-activity-send.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

void
level0SendActivities
(
  const std::deque<gpu_activity_t*>& activities
) 
{
  // Cache to store channels keyed by thread id
  std::unordered_map<uint32_t, gpu_activity_channel_t*> channelCache;

  // Iterate over each activity and send it to the corresponding channel
  for (const auto* activity : activities) {
    // Extract the thread id from the activity's correlation id
    uint32_t thread_id = gpu_activity_channel_correlation_id_get_thread_id(activity->details.instruction.correlation_id);

    // Try to find the channel in the cache
    gpu_activity_channel_t* channel = nullptr;
    auto it = channelCache.find(thread_id);
    if (it != channelCache.end()) {
      channel = it->second;
    } else {
      // If not found, look up the channel and add it to the cache
      channel = gpu_activity_channel_lookup(thread_id);
      channelCache.emplace(thread_id, channel);
    }

    // Send the activity to the found channel
    gpu_activity_channel_send(channel, activity);
  }
}
