// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

//*****************************************************************************
// local includes
//*****************************************************************************

#include "level0-module.hpp"


//******************************************************************************
// interface operations
//******************************************************************************

std::string
zeroGetKernelName
(
  ze_kernel_handle_t kernel
)
{
  size_t name_len = 0;
  ze_result_t status = zeKernelGetName(kernel, &name_len, nullptr);
  if (status != ZE_RESULT_SUCCESS || name_len == 0) {
    return "UnknownKernel";
  }

  std::vector<char> kernel_name(name_len);
  status = zeKernelGetName(kernel, &name_len, kernel_name.data());
  return (status == ZE_RESULT_SUCCESS) ? std::string(kernel_name.begin(), kernel_name.end() - 1) : "UnknownKernel";
}

uint64_t
zeroGetFunctionPointer
(
  ze_module_handle_t module,
  const std::string& kernel_name
)
{
  void* function_pointer = nullptr;
  ze_result_t status = zeModuleGetFunctionPointer(module, kernel_name.c_str(), &function_pointer);
  
  if (status == ZE_RESULT_SUCCESS && function_pointer != nullptr) {
    return reinterpret_cast<uint64_t>(function_pointer);
  } else {
    std::cerr << "[WARNING] Unable to get function pointer for kernel: " << kernel_name << std::endl;
    return 0;
  }
}

std::vector<uint8_t>
zeroGetModuleDebugInfo
(
  ze_module_handle_t module
)
{
  size_t binary_size = 0;
  ze_result_t status = zetModuleGetDebugInfo(
    module, 
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &binary_size, 
    nullptr
  );
  level0_check_result(status, __LINE__);

  if (binary_size == 0) {
    std::cerr << "[WARNING] Unable to find kernel symbols" << std::endl;
    return {};
  }

  std::vector<uint8_t> binary(binary_size);
  status = zetModuleGetDebugInfo(
    module, 
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &binary_size, 
    binary.data()
  );
  level0_check_result(status, __LINE__);

  return binary;
}

std::vector<std::string>
zeroGetModuleKernelNames
(
  ze_module_handle_t module
)
{
  uint32_t kernel_count = 0;
  ze_result_t status = zeModuleGetKernelNames(module, &kernel_count, nullptr);
  
  if (status != ZE_RESULT_SUCCESS || kernel_count == 0) {
    std::cerr << "[WARNING] Unable to get kernel count, status: " << status << std::endl;
    return {};
  }

  std::vector<const char*> kernel_names(kernel_count);
  status = zeModuleGetKernelNames(module, &kernel_count, kernel_names.data());
  
  if (status != ZE_RESULT_SUCCESS) {
    std::cerr << "[WARNING] Unable to get kernel names, status: " << status << std::endl;
    return {};
  }

  std::vector<std::string> result;
  result.reserve(kernel_count);
  for (uint32_t i = 0; i < kernel_count; ++i) {
    result.emplace_back(kernel_names[i]);
  }

  return result;
}
