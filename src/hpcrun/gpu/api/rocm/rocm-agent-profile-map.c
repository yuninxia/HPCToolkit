// SPDX-FileCopyrightText: 2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#define _GNU_SOURCE

#include "../../../../common/lean/collections/splay-tree-entry-data.h"
#include "../../../../common/lean/spinlock.h"
#include "../../../decl-init-cast.h"


#include "rocm-agent.h"
#include "rocm-agent-profile-map.h"
#include "rocm-counter-vector.h"



//******************************************************************************
// debugging
//******************************************************************************

#define DEBUG 0

#include "../../../gpu/common/gpu-print.h"



//******************************************************************************
// generic code - splay tree
//******************************************************************************

typedef struct rocm_agent_profile_map_entry_t {
  SPLAY_TREE_ENTRY_DATA(struct rocm_agent_profile_map_entry_t);
  rocprofiler_agent_id_t agent_id;
  rocprofiler_profile_config_id_t profile;
} rocm_agent_profile_map_entry_t;


#define SPLAY_TREE_PREFIX         st
#define SPLAY_TREE_KEY_TYPE       rocprofiler_agent_id_t
#define SPLAY_TREE_KEY_FIELD      agent_id
#define SPLAY_TREE_ENTRY_TYPE     rocm_agent_profile_map_entry_t

#define SPLAY_TREE_LT(A, B) (A.handle < B.handle)
#define SPLAY_TREE_GT(A, B) (A.handle > B.handle)
#define SPLAY_TREE_EQ(A, B) (A.handle == B.handle)

#define SPLAY_TREE_DEFINE_INPLACE
#include "../../../../common/lean/collections/splay-tree.h"



//******************************************************************************
// private data
//******************************************************************************

static st_t agent_profile_map = SPLAY_TREE_INITIALIZER;

static spinlock_t rocm_agent_profile_map_lock;



//*****************************************************************************
// private operations
//*****************************************************************************

// Create a counter collection profile for an agent
static void
rocm_counter_agent_profile_create
(
  rocprofiler_agent_id_t agent_id,
  rocm_counter_vector_t *vector,
  rocprofiler_profile_config_id_t *profile
)
{
  rocprofiler_counter_id_t *counters = rocm_counter_vector_data(vector);
  uint64_t num_counters = rocm_counter_vector_size(vector);

  ROCPROFILER_CALL
  (
    rocprofiler_create_profile_config,
    (agent_id, counters, num_counters, profile),
    "Could not construct counter profile config"
  );
}


static void
rocm_agent_profile_map_dump_helper
(
  rocm_agent_profile_map_entry_t *entry,
  void *arg
)
{
  // FIXME
}



//*****************************************************************************
// interface operations
//*****************************************************************************

void
rocm_agent_profile_map_init
(
  void
)
{
  agent_profile_map = (st_t) SPLAY_TREE_INITIALIZER;

  spinlock_init(&rocm_agent_profile_map_lock);
}


bool
rocm_agent_profile_map_insert
(
  rocprofiler_agent_id_t agent_id,
  rocm_counter_vector_t *vector
)
{
  PRINT("rocm_agent_profile_map_insert: agent=0x%lx vector of %ld counters\n",
        rocm_agent_id_get_id(agent_id), rocm_counter_vector_size(vector));

  DECL_INIT_CAST(rocm_agent_profile_map_entry_t *, entry,
                 malloc(sizeof(rocm_agent_profile_map_entry_t)));
  entry->agent_id = agent_id;

  rocm_counter_agent_profile_create(agent_id, vector, &entry->profile);

  spinlock_lock(&rocm_agent_profile_map_lock);

  bool inserted = st_insert(&agent_profile_map, entry);

  spinlock_unlock(&rocm_agent_profile_map_lock);

  if (!inserted) {
    free(entry);
  }

  return inserted;
}


rocprofiler_profile_config_id_t *
rocm_agent_profile_map_find
(
  rocprofiler_agent_id_t agent_id
)
{
  PRINT("rocm_agent_profile_map_find: agent=0x%lx\n", rocm_agent_id_get_id(agent_id));

  spinlock_lock(&rocm_agent_profile_map_lock);

  rocm_agent_profile_map_entry_t *entry =
    st_lookup(&agent_profile_map, agent_id);

  spinlock_unlock(&rocm_agent_profile_map_lock);

  assert(entry != NULL);

  return &entry->profile;
}


void
rocm_agent_profile_map_dump
(
  void
)
{
  PRINT("rocm_agent_profile_map_dump:\n");

  spinlock_lock(&rocm_agent_profile_map_lock);

  st_for_each(&agent_profile_map, rocm_agent_profile_map_dump_helper, 0);

  spinlock_unlock(&rocm_agent_profile_map_lock);
}
