// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//*****************************************************************************
// local includes
//*****************************************************************************

#define _GNU_SOURCE

#include "../../../../../common/lean/hpcrun-fmt.h"
#include "../../../../../common/lean/spinlock.h"
#include "../../../../../common/lean/splay-macros.h"

#include "../../../../messages/messages.h"
#include "../../../../memory/hpcrun-malloc.h"

#include "level0-id-map.h"


//*****************************************************************************
// macros
//*****************************************************************************

#define LEVEL0_ZEBIN_ID_MAP_HASH_TABLE_SIZE 127


//*****************************************************************************
// local variables
//*****************************************************************************

static zebin_id_map_entry_t *zebin_id_map_root = NULL;
static spinlock_t zebin_id_map_lock = SPINLOCK_UNLOCKED;


//*****************************************************************************
// private operations
//*****************************************************************************

static zebin_id_map_entry_t *
zebin_id_map_entry_new
(
 uint32_t zebin_id,
 SymbolVector *vector,
 uint32_t hpctoolkit_module_id
)
{
  zebin_id_map_entry_t *e;
  e = (zebin_id_map_entry_t *)hpcrun_malloc_safe(sizeof(zebin_id_map_entry_t));
  e->zebin_id = zebin_id;
  e->hpctoolkit_module_id = hpctoolkit_module_id;
  e->load_module_unused = true; // unused until a kernel is invoked
  e->left = NULL;
  e->right = NULL;
  e->elf_vector = vector;

  return e;
}


static zebin_id_map_entry_t *
zebin_id_map_splay(zebin_id_map_entry_t *root, uint32_t key)
{
  REGULAR_SPLAY_TREE(zebin_id_map_entry_s, root, key, zebin_id, left, right);
  return root;
}


static void
zebin_id_map_delete_root()
{
  TMSG(DEFER_CTXT, "zebin_id %d: delete", zebin_id_map_root->zebin_id);

  if (zebin_id_map_root->left == NULL) {
    zebin_id_map_root = zebin_id_map_root->right;
  } else {
    zebin_id_map_root->left =
      zebin_id_map_splay(zebin_id_map_root->left,
        zebin_id_map_root->zebin_id);
    zebin_id_map_root->left->right = zebin_id_map_root->right;
    zebin_id_map_root = zebin_id_map_root->left;
  }
}


//*****************************************************************************
// interface operations
//*****************************************************************************

zebin_id_map_entry_t *
zebin_id_map_lookup
(
 uint32_t id
)
{
  zebin_id_map_entry_t *result = NULL;

  static __thread zebin_id_map_hash_entry_t *zebin_id_map_hash_table = NULL;

  if (zebin_id_map_hash_table == NULL) {
    zebin_id_map_hash_table = (zebin_id_map_hash_entry_t *)hpcrun_malloc_safe(
      LEVEL0_ZEBIN_ID_MAP_HASH_TABLE_SIZE * sizeof(zebin_id_map_hash_entry_t));
    memset(zebin_id_map_hash_table, 0, LEVEL0_ZEBIN_ID_MAP_HASH_TABLE_SIZE *
      sizeof(zebin_id_map_hash_entry_t));
  }

  zebin_id_map_hash_entry_t *hash_entry = &(zebin_id_map_hash_table[id % LEVEL0_ZEBIN_ID_MAP_HASH_TABLE_SIZE]);

  if (hash_entry->entry == NULL || hash_entry->zebin_id != id) {
    spinlock_lock(&zebin_id_map_lock);

    zebin_id_map_root = zebin_id_map_splay(zebin_id_map_root, id);
    if (zebin_id_map_root && zebin_id_map_root->zebin_id == id) {
      result = zebin_id_map_root;
    }

    spinlock_unlock(&zebin_id_map_lock);

    hash_entry->zebin_id = id;
    hash_entry->entry = result;
  } else {
    result = hash_entry->entry;
  }

  TMSG(DEFER_CTXT, "zebin_id map lookup: id=0x%lx (record %p)", id, result);
  return result;
}


void
zebin_id_map_insert
(
 uint32_t zebin_id,
 uint32_t hpctoolkit_module_id,
 SymbolVector *vector
)
{
  spinlock_lock(&zebin_id_map_lock);

  if (zebin_id_map_root != NULL) {
    zebin_id_map_root =
      zebin_id_map_splay(zebin_id_map_root, zebin_id);

    if (zebin_id < zebin_id_map_root->zebin_id) {
      zebin_id_map_entry_t *entry =
        zebin_id_map_entry_new(zebin_id, vector, hpctoolkit_module_id);
      TMSG(DEFER_CTXT, "zebin_id map insert: id=0x%lx (record %p)", zebin_id, entry);
      entry->left = entry->right = NULL;
      entry->left = zebin_id_map_root->left;
      entry->right = zebin_id_map_root;
      zebin_id_map_root->left = NULL;
      zebin_id_map_root = entry;
    } else if (zebin_id > zebin_id_map_root->zebin_id) {
      zebin_id_map_entry_t *entry =
        zebin_id_map_entry_new(zebin_id, vector, hpctoolkit_module_id);
      TMSG(DEFER_CTXT, "zebin_id map insert: id=0x%lx (record %p)", zebin_id, entry);
      entry->left = entry->right = NULL;
      entry->left = zebin_id_map_root;
      entry->right = zebin_id_map_root->right;
      zebin_id_map_root->right = NULL;
      zebin_id_map_root = entry;
    } else {
      // zebin_id already present
    }
  } else {
    zebin_id_map_entry_t *entry =
      zebin_id_map_entry_new(zebin_id, vector, hpctoolkit_module_id);
    zebin_id_map_root = entry;
  }

  spinlock_unlock(&zebin_id_map_lock);
}


void
zebin_id_map_delete
(
 uint32_t zebin_id
)
{
  zebin_id_map_root = zebin_id_map_splay(zebin_id_map_root, zebin_id);
  if (zebin_id_map_root && zebin_id_map_root->zebin_id == zebin_id) {
    zebin_id_map_delete_root();
  }
}


uint32_t
zebin_id_map_entry_hpctoolkit_id_get
(
 zebin_id_map_entry_t *entry
)
{
  return entry->hpctoolkit_module_id;
}


SymbolVector *
zebin_id_map_entry_elf_vector_get
(
 zebin_id_map_entry_t *entry
)
{
  return entry->elf_vector;
}


char* 
level0_kernel_name_get
(
  ze_kernel_handle_t hKernel
) 
{
  size_t name_len = 0;
  ze_result_t status = zeKernelGetName(hKernel, &name_len, NULL);
  if (status != ZE_RESULT_SUCCESS || name_len == 0) {
    fprintf(stderr, "zeKernelGetName failed or returned zero length\n");
    return NULL;
  }

  char* kernel_name = (char*) malloc(name_len);
  status = zeKernelGetName(hKernel, &name_len, kernel_name);
  if (status != ZE_RESULT_SUCCESS) {
    fprintf(stderr, "zeKernelGetName failed\n");
    free(kernel_name);
    return NULL;
  }
  
  return kernel_name;
}


uint8_t* 
level0_module_debug_zebin_get
(
  ze_module_handle_t hModule, 
  size_t* zebin_size
) 
{
  zetModuleGetDebugInfo(
    hModule,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    zebin_size,
    NULL
  );

  uint8_t* debug_zebin = (uint8_t*) malloc(*zebin_size);
  zetModuleGetDebugInfo(
    hModule,
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,
    zebin_size,
    debug_zebin
  );

  return debug_zebin;
}


//--------------------------------------------------------------------------
// Transform a <hModule, hKernel, offset> tuple to a pc address by
// using level0 api zeModuleGetFunctionPointer
//--------------------------------------------------------------------------
ip_normalized_t 
zebin_id_transform
(
  ze_module_handle_t hModule, 
  ze_kernel_handle_t hKernel, 
  uint64_t offset
) 
{
  ip_normalized_t ip = {0, 0};
  
  char* function_name = level0_kernel_name_get(hKernel);
  if (!function_name) {
    return ip;
  }

  size_t debug_zebin_size;
  uint8_t* debug_zebin = level0_module_debug_zebin_get(hModule, &debug_zebin_size);

  char module_id[CRYPTO_HASH_STRING_LENGTH];
  crypto_compute_hash_string(debug_zebin, debug_zebin_size, module_id, CRYPTO_HASH_STRING_LENGTH);
  
  uint32_t module_id_uint32;
  sscanf(module_id, "%8x", &module_id_uint32);
  
  zebin_id_map_entry_t *entry = zebin_id_map_lookup(module_id_uint32);
  TMSG(LEVEL0, "zebin_id %d", module_id_uint32);
  if (entry != NULL) {
    uint32_t hpctoolkit_module_id = zebin_id_map_entry_hpctoolkit_id_get(entry);
    TMSG(LEVEL0, "get hpctoolkit_module_id %d", hpctoolkit_module_id);
    ip.lm_id = (uint16_t)hpctoolkit_module_id;

    void* function_pointer = NULL;
    ze_result_t status = zeModuleGetFunctionPointer(hModule, function_name, &function_pointer);
    if (status == ZE_RESULT_SUCCESS && function_pointer != NULL) {
      ip.lm_ip = (uintptr_t)function_pointer + offset;
    } else {
      fprintf(stderr, "zeModuleGetFunctionPointer failed for function %s\n", function_name);
      free(function_name);
      free(debug_zebin);
      return ip;
    }

    if (entry->load_module_unused) {
      hpcrun_loadmap_lock();
      load_module_t *lm = hpcrun_loadmap_findById(ip.lm_id);
      if (lm) {
        hpcrun_loadModule_flags_set(lm, LOADMAP_ENTRY_ANALYZE);
        entry->load_module_unused = false;
      }
      hpcrun_loadmap_unlock();
    }
  }

  free(function_name);
  free(debug_zebin);
  return ip;
}


//*****************************************************************************
// debugging code
//*****************************************************************************

static int
zebin_id_map_count_helper
(
 zebin_id_map_entry_t *entry
)
{
  if (entry) {
    int left = zebin_id_map_count_helper(entry->left);
    int right = zebin_id_map_count_helper(entry->right);
    return 1 + right + left;
  }
  return 0;
}


int
zebin_id_map_count
(
 void
)
{
  return zebin_id_map_count_helper(zebin_id_map_root);
}
