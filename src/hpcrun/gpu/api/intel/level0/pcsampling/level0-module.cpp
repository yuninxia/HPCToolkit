// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
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
level0GetKernelName
(
  ze_kernel_handle_t kernel,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  size_t name_len = 0;
  // First call to determine the required buffer size
  ze_result_t status = f_zeKernelGetName(kernel, &name_len, nullptr, dispatch);
  if (status != ZE_RESULT_SUCCESS || name_len == 0) {
    return "UnknownKernel";
  }

  // Allocate a buffer for the kernel name (including the null terminator)
  std::vector<char> kernel_name(name_len);
  status = f_zeKernelGetName(kernel, &name_len, kernel_name.data(), dispatch);
  
  // Construct a std::string excluding the null terminator if successful
  return (status == ZE_RESULT_SUCCESS) ? std::string(kernel_name.begin(), kernel_name.end() - 1) : "UnknownKernel";
}

uint64_t
level0GetFunctionPointer
(
  ze_module_handle_t module,
  const std::string& kernel_name,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  void* function_pointer = nullptr;
  ze_result_t status = f_zeModuleGetFunctionPointer(module, kernel_name.c_str(), &function_pointer, dispatch);
  
  if (status == ZE_RESULT_SUCCESS && function_pointer != nullptr) {
    return reinterpret_cast<uint64_t>(function_pointer);
  } else {
    std::cerr << "[WARNING] Unable to get function pointer for kernel: " << kernel_name << std::endl;
    return 0;
  }
}

std::vector<uint8_t>
level0GetModuleDebugInfo
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  size_t binary_size = 0;
  ze_result_t status = f_zetModuleGetDebugInfo(
    module, 
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &binary_size, 
    nullptr,
    dispatch
  );
  level0_check_result(status, __LINE__);

  if (binary_size == 0) {
    std::cerr << "[WARNING] Unable to find kernel symbols" << std::endl;
    return {};
  }

  std::vector<uint8_t> binary(binary_size);
  status = f_zetModuleGetDebugInfo(
    module, 
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &binary_size, 
    binary.data(),
    dispatch
  );
  level0_check_result(status, __LINE__);

  return binary;
}

std::vector<std::string>
level0GetModuleKernelNames
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  uint32_t kernel_count = 0;
  ze_result_t status = f_zeModuleGetKernelNames(module, &kernel_count, nullptr, dispatch);
  
  if (status != ZE_RESULT_SUCCESS || kernel_count == 0) {
    std::cerr << "[WARNING] Unable to get kernel count, status: " << status << std::endl;
    return {};
  }

  std::vector<const char*> kernel_names(kernel_count);
  status = f_zeModuleGetKernelNames(module, &kernel_count, kernel_names.data(), dispatch);
  
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
