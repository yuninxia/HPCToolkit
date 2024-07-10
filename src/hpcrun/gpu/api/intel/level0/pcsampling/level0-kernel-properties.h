#ifndef LEVEL0_KERNEL_PROPERTIES_H
#define LEVEL0_KERNEL_PROPERTIES_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
struct KernelProperties {
  std::string name;
  uint64_t base_address;
  std::string kernel_id;
  std::string module_id;
  size_t size;
  size_t sample_count;
};

void
zeroReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name,
  std::map<uint64_t, KernelProperties>& kprops
);

#endif // LEVEL0_KERNEL_PROPERTIES_H