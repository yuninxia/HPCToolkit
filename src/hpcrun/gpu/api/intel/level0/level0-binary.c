// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// system includes
//*****************************************************************************

#define _GNU_SOURCE

#include <stdlib.h>
#include <limits.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../common/gpu-binary.h"

#include "../../../../messages/messages.h"

#include "../../../../../common/lean/crypto-hash.h"

#include "level0-binary.h"



//******************************************************************************
// interface operations
//******************************************************************************

void
level0_binary_process
(
  ze_module_handle_t module,
  const struct hpcrun_foil_appdispatch_level0* dispatch
)
{
  // Get the debug binary
  size_t size;
  f_zetModuleGetDebugInfo(
    module,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &size,
    NULL,
    dispatch
  );

  uint8_t* buf = (uint8_t*) malloc(size);
  f_zetModuleGetDebugInfo(
    module,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &size,
    buf,
    dispatch
  );

  uint32_t loadmap_module_id;
  gpu_binary_save(buf, size, true /* mark_used */, &loadmap_module_id);

  // Generate a hash for the binary
  char *hash_buf = (char *) malloc(CRYPTO_HASH_STRING_LENGTH);
  crypto_compute_hash_string(buf, size, hash_buf, CRYPTO_HASH_STRING_LENGTH);

  gpu_binary_kind_t bkind = gpu_binary_kind((const char *) buf, size);

  switch (bkind){
  case gpu_binary_kind_intel_patch_token:
    TMSG(LEVEL0, "INFO: hpcrun Level Zero binary kind: Intel Patch Token");
    break;
  case gpu_binary_kind_elf:
    TMSG(LEVEL0, "INFO: hpcrun Level Zero binary kind: ELF");
    break;
  case gpu_binary_kind_empty:
    TMSG(LEVEL0, "WARNING: hpcrun: Level Zero presented an empty GPU binary.\n"
         "Instruction-level may not be possible for kernels in this binary");
    break;
  case gpu_binary_kind_unknown:
    {
    const char *magic = (const char *) buf;
    TMSG(LEVEL0, "WARNING: hpcrun: Level Zero presented unknown binary kind: magic number='%c%c%c%c'\n"
         "Instruction-level may not be possible for kernels in this binary",
          magic[0], magic[1], magic[2], magic[3]);
    }
    break;
  case gpu_binary_kind_malformed:
    TMSG(LEVEL0, "WARNING: hpcrun: Level Zero presented a malformed GPU binary.\n"
         "Instruction-level may not be possible for kernels in this binary");
    break;
  }
}
