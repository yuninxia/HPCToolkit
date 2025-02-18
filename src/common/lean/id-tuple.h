// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// Purpose:
//   Low-level types and functions for reading/writing id_tuples (each represent a unique profile)
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef HPCTOOLKIT_PROF_LEAN_ID_TUPLE_H
#define HPCTOOLKIT_PROF_LEAN_ID_TUPLE_H

//***************************************************************************
// system includes
//***************************************************************************

#include <stdbool.h>
#include <limits.h>



//***************************************************************************
// local includes
//***************************************************************************


#include "hpcio.h"
#include "hpcio-buffer.h"
#include "hpcfmt.h"



//***************************************************************************
// macros
//***************************************************************************

#define IDTUPLE_INVALID             UINT16_MAX

#define IDTUPLE_SUMMARY             0
#define IDTUPLE_NODE                1
#define IDTUPLE_RANK                2
#define IDTUPLE_THREAD              3
#define IDTUPLE_GPUDEVICE           4
#define IDTUPLE_GPUCONTEXT          5
#define IDTUPLE_GPUSTREAM           6
#define IDTUPLE_CORE                7


#define IDTUPLE_MAXTYPES            8

#define PMS_id_tuple_len_SIZE       2
#define PMS_id_SIZE                 18

#define IDTUPLE_IDS_BOTH_VALID      0
#define IDTUPLE_IDS_LOGIC_LOCAL     1
#define IDTUPLE_IDS_LOGIC_GLOBAL    2
#define IDTUPLE_IDS_LOGIC_ONLY      3

#define IDTUPLE_GET_INTERPRET(kind) (((kind)>>14) & 0x3)
#define IDTUPLE_GET_KIND(kind)      ((kind) & ((1<<14)-1))
#define IDTUPLE_COMPOSE(kind, intr) (((uint16_t)(intr) << 14) | (kind))

//***************************************************************************
// types
//***************************************************************************

typedef struct pms_id_t {
  uint16_t kind;
  uint64_t physical_index;
  uint64_t logical_index;
} pms_id_t;


typedef struct id_tuple_t {
  uint16_t length; // number of valid ids

  uint16_t ids_length; // number of id slots allocated
  pms_id_t* ids;
} id_tuple_t;


typedef void* (id_tuple_allocator_fn_t)(size_t bytes);



//***************************************************************************
// interface operations
//***************************************************************************

#if defined(__cplusplus)
extern "C" {
#endif



const char *
kindStr
(
  const uint16_t kind
);

//---------------------------------------------------------------------------
// tuple initialization
//---------------------------------------------------------------------------

void
id_tuple_constructor
(
 id_tuple_t *tuple,
 pms_id_t *ids,
 int ids_length
);


void
id_tuple_push_back
(
 id_tuple_t *tuple,
 uint16_t kind,
 uint64_t physical_index,
 uint64_t logical_index
);


void
id_tuple_copy
(
 id_tuple_t *dest,
 id_tuple_t *src,
 id_tuple_allocator_fn_t alloc
);


//---------------------------------------------------------------------------
// Single id_tuple
//---------------------------------------------------------------------------

int
id_tuple_fwrite(id_tuple_t* x, FILE* fs);

int
id_tuple_fread(id_tuple_t* x, FILE* fs);

int
id_tuple_fprint(id_tuple_t* x, FILE* fs);

void
id_tuple_dump(id_tuple_t* x);

void
id_tuple_free(id_tuple_t* x);


//---------------------------------------------------------------------------
// for profile.db
//---------------------------------------------------------------------------

int
id_tuples_pms_fwrite(uint32_t num_tuples, id_tuple_t* x, FILE* fs);

int
id_tuples_pms_fread(id_tuple_t** x, uint32_t num_tuples,FILE* fs);

int
id_tuples_pms_fprint(uint32_t num_tuples,uint64_t id_tuples_size, id_tuple_t* x, FILE* fs);

void
id_tuples_pms_free(id_tuple_t** x, uint32_t num_tuples);

//---------------------------------------------------------------------------

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif //HPCTOOLKIT_PROF_LEAN_ID_TUPLE_H
