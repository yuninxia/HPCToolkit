// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#ifndef level0_id_map_h
#define level0_id_map_h


//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../../common/lean/crypto-hash.h"
#include "../../../../sample_event.h"
#include "../binaries/zebinSymbols.h"



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

typedef struct zebin_id_map_entry_s zebin_id_map_entry_t;

typedef struct {
  uint32_t zebin_id;
  zebin_id_map_entry_t *entry;
} zebin_id_map_hash_entry_t;


//*****************************************************************************
// interface operations
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

zebin_id_map_entry_t *
zebin_id_map_lookup
(
 uint32_t zebin_id
);


void
zebin_id_map_insert
(
 uint32_t zebin_id,
 uint32_t hpctoolkit_module_id,
 SymbolVector *vector
);


void
zebin_id_map_delete
(
 uint32_t zebin_id
);


uint32_t
zebin_id_map_entry_hpctoolkit_id_get
(
 zebin_id_map_entry_t *entry
);


SymbolVector *
zebin_id_map_entry_elf_vector_get
(
 zebin_id_map_entry_t *entry
);

uint8_t* 
level0_module_debug_zebin_get
(
  ze_module_handle_t hModule, 
  size_t* zebin_size
);


char* 
level0_kernel_name_get
(
  ze_kernel_handle_t hKernel
);


ip_normalized_t 
zebin_id_transform
(
  ze_module_handle_t hModule, 
  ze_kernel_handle_t hKernel, 
  uint64_t offset
);


#ifdef __cplusplus
}
#endif


#endif
