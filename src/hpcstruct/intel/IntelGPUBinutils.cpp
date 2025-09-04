// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//***************************************************************************

//******************************************************************************
// system includes
//******************************************************************************

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>




//******************************************************************************
// local includes
//******************************************************************************

#include "../../common/lean/crypto-hash.h"
#include "../ElfHelper.hpp"
#include "../../common/diagnostics.h"
#include "../../common/RealPathMgr.hpp"
#include "IntelGPUBinutils.hpp"


//******************************************************************************
// macros
//******************************************************************************

#define DBG 1

#define INTEL_GPU_DEBUG_SECTION_NAME "Intel(R) OpenCL Device Debug"



//******************************************************************************
// private operations
//******************************************************************************

static __attribute__((unused)) const char *
opencl_elf_section_type
(
  Elf64_Word sh_type
)
{
  switch (sh_type) {
    case SHT_OPENCL_SOURCE:
      return "SHT_OPENCL_SOURCE";
    case SHT_OPENCL_HEADER:
      return "SHT_OPENCL_HEADER";
    case SHT_OPENCL_LLVM_TEXT:
      return "SHT_OPENCL_LLVM_TEXT";
    case SHT_OPENCL_LLVM_BINARY:
      return "SHT_OPENCL_LLVM_BINARY";
    case SHT_OPENCL_LLVM_ARCHIVE:
      return "SHT_OPENCL_LLVM_ARCHIVE";
    case SHT_OPENCL_DEV_BINARY:
      return "SHT_OPENCL_DEV_BINARY";
    case SHT_OPENCL_OPTIONS:
      return "SHT_OPENCL_OPTIONS";
    case SHT_OPENCL_PCH:
      return "SHT_OPENCL_PCH";
    case SHT_OPENCL_DEV_DEBUG:
      return "SHT_OPENCL_DEV_DEBUG";
    case SHT_OPENCL_SPIRV:
      return "SHT_OPENCL_SPIRV";
    case SHT_OPENCL_NON_COHERENT_DEV_BINARY:
      return "SHT_OPENCL_NON_COHERENT_DEV_BINARY";
    case SHT_OPENCL_SPIRV_SC_IDS:
      return "SHT_OPENCL_SPIRV_SC_IDS";
    case SHT_OPENCL_SPIRV_SC_VALUES:
      return "SHT_OPENCL_SPIRV_SC_VALUES";
    default:
      return "unknown type";
  }
}
