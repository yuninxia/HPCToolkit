#include "level0-kernel-properties.h"
#include <filesystem>
#include <fstream>
#include <iostream>

std::map<uint64_t, KernelProperties> 
ReadKernelProperties
(
  const int32_t device_id,
  const std::string& data_dir_name
)
{
  std::map<uint64_t, KernelProperties> kprops;
  // enumerate all kernel property files
  for (const auto& e : std::filesystem::directory_iterator(std::filesystem::path(data_dir_name))) {
    // kernel properties file path: <data_dir>/.kprops.<device_id>.<pid>.txt
    if (e.path().filename().string().find(".kprops." + std::to_string(device_id)) == 0) {
      std::ifstream kpf = std::ifstream(e.path());
      if (!kpf.is_open()) {
        continue;
      }

      while (!kpf.eof()) {
        KernelProperties props;
        std::string line;

        std::getline(kpf, props.name);
        if (kpf.eof()) {
          break;
        }

        std::getline(kpf, line);
        if (kpf.eof()) {
          break;
        }
        props.base_address = std::strtol(line.c_str(), nullptr, 0);

        std::getline(kpf, props.kernel_id);
        std::getline(kpf, props.module_id);

        line.clear();
        std::getline(kpf, line);
        props.size = std::strtol(line.c_str(), nullptr, 0);

        kprops.insert({props.base_address, std::move(props)});
      }
      kpf.close();
    }
  }
  return kprops;
}