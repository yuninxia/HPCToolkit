//*****************************************************************************
// system includes
//*****************************************************************************




//*****************************************************************************
// local includes
//*****************************************************************************

#define _GNU_SOURCE

#include "../../../../../../lib/prof-lean/hpcrun-fmt.h"
#include "../../../../../../lib/prof-lean/spinlock.h"
#include "../../../../../../lib/prof-lean/splay-macros.h"
#include "../../../../messages/messages.h"
#include "../../../../memory/hpcrun-malloc.h"

#include "level0-id-map.h"



//*****************************************************************************
// macros
//*****************************************************************************


#define LEVEL0_ZEBIN_ID_MAP_HASH_TABLE_SIZE 127



//*****************************************************************************
// type definitions
//*****************************************************************************

struct zebin_id_map_entry_s {
  uint32_t zebin_id;
  uint32_t hpctoolkit_module_id;
  bool load_module_unused;
  SymbolVector *elf_vector;
  struct zebin_id_map_entry_s *left;
  struct zebin_id_map_entry_s *right;
};


typedef struct {
  uint32_t zebin_id;
  zebin_id_map_entry_t *entry;
} zebin_id_map_hash_entry_t;



//*****************************************************************************
// global data
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


//--------------------------------------------------------------------------
// Transform a <zebin_id, function_index, offset> tuple to a pc address by
// looking up elf symbols inside a zebin
//--------------------------------------------------------------------------
ip_normalized_t
zebin_id_transform(uint32_t zebin_id, char* function_name, uint64_t offset)
{
  ip_normalized_t ip = {0, 0};
  zebin_id_map_entry_t *entry = zebin_id_map_lookup(zebin_id);

  TMSG(LEVEL0, "zebin_id %d", zebin_id);
  if (entry != NULL) {
    uint32_t hpctoolkit_module_id = zebin_id_map_entry_hpctoolkit_id_get(entry);
    TMSG(LEVEL0, "get hpctoolkit_module_id %d", hpctoolkit_module_id);
    const SymbolVector *vector = zebin_id_map_entry_elf_vector_get(entry);
    ip.lm_id = (uint16_t)hpctoolkit_module_id;
    // ip.lm_ip = (uintptr_t)(vector->symbols[function_index] + offset);
    for (int i = 0; i < vector->nsymbols; i++) {
      if (strcmp(vector->symbolName[i], function_name) == 0) {
        ip.lm_ip = (uintptr_t)(vector->symbolValue[i] + offset);
        break;
      }
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
