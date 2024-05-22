// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

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
#include "../../../../../common/lean/spinlock.h"

#include "level0-binary.h"
#include "level0-handle-map.h"
#include "level0-id-map.h"
#include "level0-kernel-module-map.h"


//*****************************************************************************
// type declarations
//*****************************************************************************

typedef struct {
  const char *hash_string;
  gpu_binary_kind_t bkind;
} module_info_t;



//******************************************************************************
// local variables
//******************************************************************************

static level0_handle_map_entry_t *module_map_root = NULL;

static level0_handle_map_entry_t *module_free_list = NULL;

static spinlock_t module_lock = SPINLOCK_UNLOCKED;



//******************************************************************************
// private operations
//******************************************************************************

module_info_t *
module_info_new
(
  const char *hash_string,
  gpu_binary_kind_t bkind
)
{
  module_info_t *mi = (module_info_t *) malloc(sizeof(module_info_t));

  mi->hash_string = hash_string;
  mi->bkind = bkind;

  return mi;
}


void
level0_module_handle_map_insert
(
  ze_module_handle_t module,
  const char* hash_string,
  gpu_binary_kind_t bkind
)
{
  spinlock_lock(&module_lock);

  uint64_t key = (uint64_t)module;
  level0_handle_map_entry_t *entry =
    level0_handle_map_entry_new(&module_free_list, key, (level0_data_node_t*) module_info_new(hash_string, bkind));
  level0_handle_map_insert(&module_map_root, entry);

  spinlock_unlock(&module_lock);
}



//******************************************************************************
// interface operations
//******************************************************************************

void
level0_binary_process
(
  ze_module_handle_t module
)
{
  // Get the debug binary
  size_t debug_zebin_size;
  zetModuleGetDebugInfo(
    module,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &debug_zebin_size,
    NULL
  );

  uint8_t* debug_zebin = (uint8_t*) malloc(debug_zebin_size);
  zetModuleGetDebugInfo(
    module,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    &debug_zebin_size,
    debug_zebin
  );

  uint32_t loadmap_module_id;
  gpu_binary_save(debug_zebin, debug_zebin_size, true /* mark_used */, &loadmap_module_id);

  // Generate a hash for the binary
  char zebin_id[CRYPTO_HASH_STRING_LENGTH];
  crypto_compute_hash_string(debug_zebin, debug_zebin_size, zebin_id, CRYPTO_HASH_STRING_LENGTH);
     
  TMSG(LEVEL0, "zebin_id %d -> loadmap_module_id %d", zebin_id, loadmap_module_id);

  uint32_t zebin_id_uint32;
  sscanf(zebin_id, "%8x", &zebin_id_uint32);

  zebin_id_map_entry_t *entry = zebin_id_map_lookup(zebin_id_uint32);
  if (entry == NULL) {
    SymbolVector *symbols = collectZebinSymbols(debug_zebin, debug_zebin_size);
    zebin_id_map_insert(zebin_id_uint32, loadmap_module_id, symbols);
    free(symbols->symbolValue);
    free(symbols->symbolName);
    free(symbols);
  }

  gpu_binary_kind_t bkind = gpu_binary_kind((const char *) debug_zebin, debug_zebin_size);

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
    const char *magic = (const char *) debug_zebin;
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

  level0_module_handle_map_insert(module, zebin_id, bkind);

  free(debug_zebin);
}


void
level0_module_handle_map_lookup
(
  ze_module_handle_t module,
  const char **hash_string,
  gpu_binary_kind_t *bkind
)
{
  spinlock_lock(&module_lock);

  uint64_t key = (uint64_t)module;
  level0_handle_map_entry_t *entry =
    level0_handle_map_lookup(&module_map_root, key);
  module_info_t  *mi = (module_info_t *) (*level0_handle_map_entry_data_get(entry));
  spinlock_unlock(&module_lock);

  *hash_string = mi->hash_string;
  *bkind = mi->bkind;
}

void
level0_module_handle_map_delete
(
  ze_module_handle_t module
)
{
  spinlock_lock(&module_lock);

  uint64_t key = (uint64_t)module;
  level0_handle_map_delete(
    &module_map_root,
    &module_free_list,
    key
  );

  spinlock_unlock(&module_lock);
}

ip_normalized_t
level0_func_ip_resolve
(
  ze_kernel_handle_t hKernel
)
{
  ze_module_handle_t hModule = level0_kernel_module_map_lookup(hKernel);
  
  ip_normalized_t ip_norm = zebin_id_transform(hModule, hKernel, 0);

  return ip_norm;
}
