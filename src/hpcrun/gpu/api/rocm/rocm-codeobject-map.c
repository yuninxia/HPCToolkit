// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../activity/gpu-op-placeholders.h"
#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../../common/lean/collections/freelist-entry-data.h"
#include "../../../../common/lean/spinlock.h"

#include "rocm-codeobject-map.h"



//******************************************************************************
// debugging support
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// generic code - splay tree and freelist
//******************************************************************************

#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR == 4)
typedef struct address_range_t {
  uint64_t begin, end;
} address_range_t;

#define key_field_t address_range_t
#define key_field address_range

#define SPLAY_TREE_LT(A, B) (LB(A) < LB(B))
#define SPLAY_TREE_GT(A, B) (LB(A) > LE(B))
#define SPLAY_TREE_EQ(A, B) (LB(A) >= LB(B) && (LB(A) < LE(B)))

#define key_field_from_codeobject(key_field_ptr, codeobject_ptr) \
    key_field_ptr->begin = codeobject_ptr->load_base; \
    key_field_ptr->end = codeobject_ptr->load_base + codeobject_ptr->load_size;

#define key_field_from_pc(key_field_ptr, pc) \
    key_field_ptr->begin = key_field_ptr->end = pc

#define ip_offset_from_pc_codeobject(pc, codeobject_ptr) \
  (pc - codeobject_ptr->load_base)

#else

typedef uint64_t loaded_code_object_id_t;

#define key_field_t loaded_code_object_id_t
#define key_field loaded_code_object_id

#define key_field_from_codeobject(key_field_ptr, codeobject_ptr) \
  *key_field_ptr = codeobject_ptr->code_object_id

#define key_field_from_pc(key_field_ptr, pc) \
  *key_field_ptr = pc.loaded_code_object_id

#define ip_offset_from_pc_codeobject(pc, codeobject_ptr) \
  pc.loaded_code_object_offset

#endif


typedef struct rocm_codeobject_map_entry_t {
  union {
    SPLAY_TREE_ENTRY_DATA(struct rocm_codeobject_map_entry_t);
    FREELIST_ENTRY_DATA(struct rocm_codeobject_map_entry_t);
  };
  key_field_t key_field;
  rocm_codeobject_info_t info;
} rocm_codeobject_map_entry_t;

#define LB(x) (x.begin)
#define LE(x) (x.end)

#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       key_field_t
#define SPLAY_TREE_KEY_FIELD      key_field
#define SPLAY_TREE_ENTRY_TYPE     rocm_codeobject_map_entry_t


#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"


#define FREELIST_ENTRY_TYPE      rocm_codeobject_map_entry_t
#include "../../../../common/lean/collections/freelist.h"



//******************************************************************************
// private data
//******************************************************************************

static st_t codeobject_map = SPLAY_TREE_INITIALIZER;
static freelist_t freelist = FREELIST_INIITALIZER(malloc);
static spinlock_t rocm_codeobject_map_lock;



//******************************************************************************
// private data
//******************************************************************************

static void
rocm_codeobject_map_dump_helper
(
  rocm_codeobject_map_entry_t *entry,
  void *arg
)
{
  fprintf(stderr, "  %lu --> %u\n", entry->info.code_object.code_object_id,
    entry->info.load_module_id);
}



//*****************************************************************************
// interface operations
//*****************************************************************************

void
rocm_codeobject_map_init
(
  void
)
{
  spinlock_init(&rocm_codeobject_map_lock);
}


bool
rocm_codeobject_map_insert
(
  rocprofiler_kernel_code_object_t *code_object,
  uint32_t load_module_id
)
{
 spinlock_lock(&rocm_codeobject_map_lock);

 rocm_codeobject_map_entry_t *entry = freelist_allocate(&freelist);

  *entry = (rocm_codeobject_map_entry_t) {
    .info.code_object = *code_object,
    .info.load_module_id = load_module_id
  };

  key_field_from_codeobject((&entry->key_field), code_object);

  bool inserted = st_insert(&codeobject_map, entry);
  if (!inserted) {
    freelist_free(&freelist, entry);
  }

  PRINT("rocm_codeobject_insert: (%lu --> %u) inserted=%d\n", code_object->code_object_id, load_module_id, inserted);

  spinlock_unlock(&rocm_codeobject_map_lock);

  return inserted;
}


rocm_codeobject_info_t *
rocm_codeobject_map_find
(
  rocprofiler_pc_t pc
)
{
  key_field_t key_field;
  key_field_from_pc((&key_field), pc);

  spinlock_lock(&rocm_codeobject_map_lock);
  rocm_codeobject_map_entry_t *entry =
    st_lookup(&codeobject_map, key_field);
  spinlock_unlock(&rocm_codeobject_map_lock);

  return entry != NULL ? &entry->info : NULL;
}


ip_normalized_t
rocm_codeobject_map_normalize
(
  rocprofiler_pc_t pc
)
{
  rocm_codeobject_info_t *info = rocm_codeobject_map_find(pc);

  if (info == 0) {
    PRINT("hpcrun: unable to normalize PC sample " PC_FORMAT "\n", PC_VALUE(pc));
    return gpu_op_placeholder_ip(gpu_placeholder_type_kernel_anon);
  }

  ip_normalized_t result;
  result.lm_id = info->load_module_id;
  result.lm_ip = ip_offset_from_pc_codeobject(pc, (&info->code_object));

  PRINT("hpcrun: normalized PC sample " PC_FORMAT "--> (%d, 0x%lx)\n", PC_VALUE(pc),
    result.lm_id, result.lm_ip);

  return result;
}


bool
rocm_codeobject_map_delete
(
 rocprofiler_kernel_code_object_t *code_object
)
{
  key_field_t key_field;
  key_field_from_codeobject((&key_field), code_object);

  spinlock_lock(&rocm_codeobject_map_lock);
  rocm_codeobject_map_entry_t *entry =
    st_delete(&codeobject_map, key_field);

  if (entry == NULL) {
    return false;
  }

  freelist_free(&freelist, entry);
  spinlock_unlock(&rocm_codeobject_map_lock);

  return true;
}


void
rocm_codeobject_map_dump
(
  void
)
{
  fprintf(stderr, "rocm_codeobject_map dump: \n");
  st_for_each(&codeobject_map, rocm_codeobject_map_dump_helper, 0);
}
