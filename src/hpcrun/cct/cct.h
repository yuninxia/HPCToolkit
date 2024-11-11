// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   A variable degree-tree for storing call stack samples.  Each node
//   may have zero or more children and each node contains a single
//   instruction pointer value.  Call stack samples are represented
//   implicitly by a path from some node x (where x may or may not be
//   a leaf node) to the tree root (with the root being the bottom of
//   the call stack).
//
//   The basic tree functionality is based on NonUniformDegreeTree.h/C
//   from HPCView/HPCTools.
//
// Description:
//    [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef cct_h
#define cct_h

//************************* System Include Files ****************************

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <ucontext.h>


//*************************** User Include Files ****************************

#include "../metrics.h"

#include "../unwind/common/backtrace.h"

#include "../utilities/ip-normalized.h"


#include "../../common/lean/hpcio.h"
#include "../../common/lean/hpcfmt.h"
#include "../../common/lean/hpcrun-fmt.h"
#include "../../common/lean/placeholders.h"

#include "cct_addr.h"

//
// Readability Macros (to facilitate coding initialization operations)
//
#define CCT_ROOT HPCRUN_PLACEHOLDER_LM, hpcrun_placeholder_root_primary
#define PARTIAL_ROOT HPCRUN_PLACEHOLDER_LM, hpcrun_placeholder_root_partial
#define ADDR_I(L)     NON_LUSH_ADDR_INI(L)
#define ADDR(L)      (cct_addr_t) NON_LUSH_ADDR_INI(L)
#define ADDR2_I(id, ip) NON_LUSH_ADDR_INI(id, ip)
#define ADDR2(id, ip) (cct_addr_t) ADDR2_I(id, ip)
#define HPCRUN_DUMMY_NODE 65534

//***************************************************************************
// Calling context tree node (abstract data type)
//***************************************************************************


#define IS_PARTIAL_ROOT(addr) \
        (addr->ip_norm.lm_id == HPCRUN_PLACEHOLDER_LM) && \
        (addr->ip_norm.lm_ip == hpcrun_placeholder_root_partial)

typedef struct cct_node_t cct_node_t;
//
// In order to associate data with a given calling context,
// cct nodes need an id type (abstract)
//

typedef cct_node_t* cct_node_id_t;

//
// Interface procedures
//

//
// Constructors
//
extern cct_node_t* hpcrun_cct_new(void);
extern cct_node_t* hpcrun_cct_new_partial(void);
extern cct_node_t* hpcrun_cct_new_special(void* addr);
extern cct_node_t* hpcrun_cct_top_new(uint16_t lmid, uintptr_t lmip);
//
// Accessor functions
//

#ifdef __cplusplus
extern "C" {
#endif

extern cct_node_t* hpcrun_cct_parent(cct_node_t* node);
extern cct_node_t* hpcrun_cct_children(cct_node_t* node);
extern cct_node_t* hpcrun_leftmost_child(cct_node_t* node);
extern int32_t hpcrun_cct_persistent_id(cct_node_t* node);
extern cct_addr_t* hpcrun_cct_addr(cct_node_t* node);

#ifdef __cplusplus
}
#endif

extern bool hpcrun_cct_is_leaf(cct_node_t* node);
extern bool hpcrun_cct_unwound(cct_node_t* node);
extern cct_node_t* hpcrun_cct_insert_path_return_leaf(cct_node_t *root, cct_node_t *path);
extern void hpcrun_cct_delete_self(cct_node_t *node);
//
// NOTE: having no children is not exactly the same as being a leaf
//       A leaf represents a full path. There might be full paths
//       that are a prefix of other full paths. So, a "leaf" can have children
//
extern bool hpcrun_cct_no_children(cct_node_t* node);
extern bool hpcrun_cct_is_root(cct_node_t* node);
extern bool hpcrun_cct_is_dummy(cct_node_t* node);

//
// Mutator functions: modify a given cct
//

#ifdef __cplusplus
extern "C" {
#endif

extern void get_cct_node_id(cct_node_t* node, uint16_t* lm_id, uintptr_t* lm_ip);
extern cct_node_t* hpcrun_cct_insert_instruction_child(cct_node_t* block, uintptr_t ip);
extern cct_node_t* hpcrun_cct_insert_ip_norm(cct_node_t* node, ip_normalized_t ip_norm, bool unwound);

#ifdef __cplusplus
}
#endif

//
// Fundamental mutation operation: insert a given addr into the
// set of children of a given cct node. Return the cct_node corresponding
// to the inserted addr [NB: if the addr is already in the node children, then
// the already present node is returned. Otherwise, a new node is created, linked in,
// and returned]
//
extern cct_node_t* hpcrun_cct_insert_addr(cct_node_t* cct, cct_addr_t* addr, bool unwound);

//
// Insert a dummy node to represent the callback function by hpcrun, which will
// be eliminated before writing out the cct.
extern cct_node_t* hpcrun_cct_insert_dummy(cct_node_t* node, uint16_t lm_ip);

//
// 2nd fundamental mutator: mark a node as "terminal". That is,
//   it is the last node of a path
//
extern void hpcrun_cct_terminate_path(cct_node_t* node);
//
// Special purpose mutator:
// This operation is somewhat akin to concatenation.
// An already constructed cct ('src') is inserted as a
// child of the 'target' cct. The addr field of the src
// cct is ASSUMED TO BE DIFFERENT FROM ANY ADDR IN target's
// child set. [Otherwise something recursive has to happen]
//
//
extern cct_node_t* hpcrun_cct_insert_node(cct_node_t* target, cct_node_t* src);

extern void hpcrun_cct_insert_path(cct_node_t ** root, cct_node_t* path);

#ifdef __cplusplus
extern "C" {
#endif

// mark a node for retention as the leaf of a traced call path.
extern void hpcrun_cct_retain(cct_node_t* x);

// check if a node was marked for retention as the leaf of a traced
// call path.
extern int hpcrun_cct_retained(cct_node_t* x);

#ifdef __cplusplus
}
#endif


// Walking functions section:
//
//     typedefs for walking functions
typedef void* cct_op_arg_t;
typedef void (*cct_op_t)(cct_node_t* cct, cct_op_arg_t arg, size_t level);

//
//     general walking functions: (client may select starting level)
//       visits every node in the cct, calling op(node, arg, level)
//       level has property that node at level n ==> children at level n+1
//     there are 2 different walking strategies:
//       1) walk the children, then walk the node
//       2) walk the node, then walk the children
//     there is no implied children ordering
//

//
// visiting order: children first, then node
//
extern void hpcrun_cct_walk_child_1st_w_level(cct_node_t* cct,
                                              cct_op_t op,
                                              cct_op_arg_t arg, size_t level);

//
// visiting order: node first, then children
//
extern void hpcrun_cct_walk_node_1st_w_level(cct_node_t* cct,
                                             cct_op_t op,
                                             cct_op_arg_t arg, size_t level);
//
// Top level walking routines:
//  frequently, a walk will start with level = 0
//  the static inline routines below implement this utility
//

static inline
void hpcrun_cct_walk_child_1st(cct_node_t* cct,
                               cct_op_t op, cct_op_arg_t arg)
{
  hpcrun_cct_walk_child_1st_w_level(cct, op, arg, 0);
}

static inline
void hpcrun_cct_walk_node_1st(cct_node_t* cct,
                              cct_op_t op, cct_op_arg_t arg)
{
  hpcrun_cct_walk_node_1st_w_level(cct, op, arg, 0);
}


//
// Special routine to walk a path represented by a cct node.
// The actual path represented by a node is list reversal of the nodes
//  linked by the parent link. So walking a path involves touching the
// path nodes in list reverse order
//
extern void hpcrun_walk_path(cct_node_t* node, cct_op_t op, cct_op_arg_t arg);

//
// Iterate through the children of a cct node.
//
extern void hpcrun_walk_children(cct_node_t* cct, cct_op_t fn, cct_op_arg_t arg);
//
// Writing operation
//
// cct2metrics_t is defined in cct2metrics.h but we cannot include this header
//  beceause cct2metrics.h includes this file (cct.h)
//
// TODO: need to declare cct2metrics_t here to avoid to cyclic inclusion
typedef struct cct2metrics_t cct2metrics_t;


int hpcrun_cct_fwrite(cct2metrics_t* cct2metrics_map,
                      cct_node_t* cct, FILE* fs, epoch_flags_t flags, hpcrun_fmt_sparse_metrics_t* sparse_metrics);

void hpcrun_cct_fwrite_errmsg_w_fn(FILE* fs, uint32_t tid, char* msg);
//
// Utilities
//
extern size_t hpcrun_cct_num_nz_nodes_and_mark_display(cct_node_t* cct, bool count_dummy,\
    cct2metrics_t **cct2metrics_map,uint64_t* num_nzval, uint32_t* num_nzcct);


//
// look up addr in the set of cct's children
// return the found node or NULL
//
extern cct_node_t* hpcrun_cct_find_addr(cct_node_t* cct, cct_addr_t* addr);
//
// Merging operation: Given 2 ccts : CCT_A, CCT_B,
//    merge means add all paths in CCT_B that are NOT in CCT_A
//    to CCT_A. For paths that are common, perform the merge operation on
//    each common node, using auxiliary arg merge_arg
//
//    NOTE: this merge operation presumes
//       cct_addr_data(CCT_A) == cct_addr_data(CCT_B)
//
typedef void* merge_op_arg_t;
typedef void (*merge_op_t)(cct_node_t* a, cct_node_t*b, merge_op_arg_t arg);

extern void hpcrun_cct_merge(cct_node_t* cct_a, cct_node_t* cct_b,
                             merge_op_t merge, merge_op_arg_t arg);




// FIXME: This should not be here vi3: allocation and free cct_node_t
extern __thread cct_node_t* cct_node_freelist_head;

cct_node_t* hpcrun_cct_node_alloc();
void hpcrun_cct_node_free(cct_node_t *cct);
// remove Children from cct
void cct_remove_my_subtree(cct_node_t* cct);






// copy cct node
cct_node_t* hpcrun_cct_copy_just_addr(cct_node_t *cct);
void hpcrun_cct_set_children(cct_node_t* cct, cct_node_t* children);
void hpcrun_cct_set_parent(cct_node_t* cct, cct_node_t* parent);


#endif // cct_h
