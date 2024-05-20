#ifndef zebin_id_map_h
#define zebin_id_map_h



//*****************************************************************************
// system includes
//*****************************************************************************

#include <stdint.h>

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../../../sample_event.h"
#include "../binaries/zebinSymbols.h"



//*****************************************************************************
// type definitions
//*****************************************************************************

typedef struct zebin_id_map_entry_s zebin_id_map_entry_t;



//*****************************************************************************
// interface operations
//*****************************************************************************

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


ip_normalized_t
zebin_id_transform
(
 uint32_t zebin_id,
 char* function_id,
 uint64_t offset
);




#endif
