// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

//******************************************************************************
// system includes
//******************************************************************************

#define _GNU_SOURCE
#include <string.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../../common/lean/collections/freelist-entry-data.h"
#include "../../../../common/lean/spinlock.h"

#include "../../../decl-init-cast.h"

#include "rocm-codeobject-map.h"
#include "rocm-symbol-map.h"


//******************************************************************************
// macros
//******************************************************************************

#if (ROCPROFILER_VERSION_MAJOR == 0 && ROCPROFILER_VERSION_MINOR == 4)

#define set_kernel_pc(pc, kernel_symbol, kernel_object) \
  *pc = kernel_symbol->kernel_object + kernel_object->kernel_code_entry_byte_offset

#else

#define set_kernel_pc(pc, kernel_symbol, kernel_object) \
  pc->loaded_code_object_id = kernel_symbol->code_object_id; \
  pc->loaded_code_object_offset =  \
    (kernel_symbol->kernel_object + \
     kernel_object->kernel_code_entry_byte_offset - \
     rocm_codeobject_map_find(*pc)->code_object.load_base)

#endif



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// generic code - splay tree and freelist
//******************************************************************************

typedef struct rocm_symbol_map_entry_t {
  union {
    SPLAY_TREE_ENTRY_DATA(struct rocm_symbol_map_entry_t);
    FREELIST_ENTRY_DATA(struct rocm_symbol_map_entry_t);
  };
  uint64_t kernel_id; // key
  rocm_symbol_info_t info;
} rocm_symbol_map_entry_t;

#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       uint64_t
#define SPLAY_TREE_KEY_FIELD      kernel_id
#define SPLAY_TREE_ENTRY_TYPE     rocm_symbol_map_entry_t

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"


#define FREELIST_ENTRY_TYPE      rocm_symbol_map_entry_t
#include "../../../../common/lean/collections/freelist.h"



//******************************************************************************
// type declarations
//******************************************************************************

// this is copied from rocprofiler-sdk/source/lib/rocprofiler-sdk/code_object/code_object.cpp
typedef struct kernel_descriptor_t {
    uint8_t  reserved0[16];
    int64_t  kernel_code_entry_byte_offset;
    uint8_t  reserved1[20];
    uint32_t compute_pgm_rsrc3;
    uint32_t compute_pgm_rsrc1;
    uint32_t compute_pgm_rsrc2;
    uint16_t kernel_code_properties;
    uint8_t  reserved2[6];
} kernel_descriptor_t;



//******************************************************************************
// private data
//******************************************************************************

static st_t symbol_map = SPLAY_TREE_INITIALIZER;
static freelist_t freelist = FREELIST_INIITALIZER(malloc);
static spinlock_t rocm_symbol_map_lock;



//******************************************************************************
// forward declarations
//******************************************************************************

static char *
rocm_symbol_trim
(
  const char *kernel_name
) __attribute__ ((unused));



//******************************************************************************
// private operations
//******************************************************************************

static char *
rocm_symbol_trim
(
  const char *kernel_name
)
{
  // rocm kernel names end in .kd; trim that off
  char *name = strdup(kernel_name);
  char *dot = strchrnul(name, '.');
  *dot = 0;
  return name;
}

static rocprofiler_pc_t
rocm_symbol_pc
(
  rocprofiler_kernel_symbol_t *kernel_symbol
)
{
  DECL_INIT_CAST(kernel_descriptor_t *, kernel_object, kernel_symbol->kernel_object);

  rocprofiler_pc_t kernel_pc;
  set_kernel_pc((&kernel_pc), kernel_symbol, kernel_object);

  return kernel_pc;
}



//******************************************************************************
// interface operations
//******************************************************************************

void
rocm_symbol_map_init
(
  void
)
{
  spinlock_init(&rocm_symbol_map_lock);
}


void
rocm_symbol_map_insert
(
  rocprofiler_kernel_symbol_t *kernel_symbol
)
{
#if DEBUG
  char *name = rocm_symbol_trim(kernel_symbol->kernel_name);
#endif

  rocprofiler_pc_t kernel_pc = rocm_symbol_pc(kernel_symbol);

  PRINT("inserting kernel id %ld --> %s " PC_FORMAT "\n",
    kernel_symbol->kernel_id, name, PC_VALUE(kernel_pc));

  spinlock_lock(&rocm_symbol_map_lock);

  rocm_symbol_map_entry_t *entry = freelist_allocate(&freelist);

  *entry = (rocm_symbol_map_entry_t) {
    .kernel_id = kernel_symbol->kernel_id,
    .info.kernel_ip = rocm_codeobject_map_normalize(kernel_pc),
    .info.kernel_arg_bytes = kernel_symbol->kernarg_segment_size,
    .info.workgroup_LDS_bytes = kernel_symbol->group_segment_size,
    .info.kernel_scratch_bytes = kernel_symbol->private_segment_size,
    .info.kernel_sgpr_count = kernel_symbol->sgpr_count,
    .info.kernel_vgpr_count = kernel_symbol->arch_vgpr_count,
    .info.total_vgpr_count = kernel_symbol->accum_vgpr_count
  };

  bool inserted = st_insert(&symbol_map, entry);
  if (!inserted) {
    freelist_free(&freelist, entry);
  }

  spinlock_unlock(&rocm_symbol_map_lock);
}


bool
rocm_symbol_map_delete
(
  rocprofiler_kernel_symbol_t *kernel_symbol
)
{
  PRINT("deleting kernel id %ld\n", kernel_symbol->kernel_id);

  spinlock_lock(&rocm_symbol_map_lock);
  rocm_symbol_map_entry_t *entry =
    st_delete(&symbol_map, kernel_symbol->kernel_id);

  if (entry == NULL) {
    return false;
  }

  freelist_free(&freelist, entry);
  spinlock_unlock(&rocm_symbol_map_lock);

  return true;
}


rocm_symbol_info_t *
rocm_symbol_map_find
(
  rocprofiler_kernel_id_t kernel_id
)
{
  spinlock_lock(&rocm_symbol_map_lock);
  rocm_symbol_map_entry_t *entry =
    st_lookup(&symbol_map, kernel_id);
  spinlock_unlock(&rocm_symbol_map_lock);

  return entry != NULL ? &(entry->info) : NULL;
}
