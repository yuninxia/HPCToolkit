#ifndef LEVEL0_ACTIVITY_TRANSLATE_H
#define LEVEL0_ACTIVITY_TRANSLATE_H

#include <deque>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#include "../../../../activity/gpu-activity.h"
#include "../level0-id-map.h"
#include "level0-kernel-properties.h"
#include "level0-metric.h"

void
zeroConvertStallReason
(
  const EuStalls& stall,
  gpu_inst_stall_t& stall_reason
);

bool
zeroConvertPCSampling
(
  gpu_activity_t* activity, 
  std::map<uint64_t, EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit, 
  uint64_t correlation_id
);

void
zeroActivityTranslate
(
  std::deque<gpu_activity_t*>& activities, 
  std::map<uint64_t, EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit, 
  uint64_t correlation_id
);

template <typename T>
T hex_string_to_uint
(
  const std::string& hex_str
) 
{
  std::stringstream ss;
  T num = 0;
  ss << std::hex << hex_str;
  ss >> num;
  return num;
}

#endif // LEVEL0_ACTIVITY_TRANSLATE_H