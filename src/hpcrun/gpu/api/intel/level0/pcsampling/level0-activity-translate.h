#ifndef LEVEL0_ACTIVITY_TRANSLATE_H
#define LEVEL0_ACTIVITY_TRANSLATE_H

#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include "../../../../activity/gpu-activity.h"
#include "level0-metric.h"
#include "level0-kernel-properties.h"

gpu_inst_stall_t
level0_convert_stall_reason
(
  const l0_metric::EuStalls& stall
);

bool
level0_convert_pcsampling
(
  gpu_activity_t* activity, 
  std::map<uint64_t, l0_metric::EuStalls>::iterator it,
  std::map<uint64_t, KernelProperties>::const_reverse_iterator rit, 
  uint64_t correlation_id
);

void
level0_activity_translate
(
  std::deque<gpu_activity_t*>& activities, 
  std::map<uint64_t, l0_metric::EuStalls>::iterator it,
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