// SPDX-FileCopyrightText: 2002-2024 Rice University
// SPDX-FileCopyrightText: 2024 Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause


//******************************************************************************

// This file takes a buffer tracing 'kind' and a mapping of regular
// expression strings to gpu activity types and computes an array of
// operation indices that match to feed into
// rocprofiler_configure_buffer_tracing_service().


//******************************************************************************
// system includes
//******************************************************************************

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <regex.h>



//******************************************************************************
// hpctoolkit includes
//******************************************************************************

#include "../../../messages/messages.h"
#include "rocm.h"
#include "rocm-regex.h"
#include "rocm-utils.h"

#define FAILURE  (-1)



//******************************************************************************
// public interfaces
//******************************************************************************

// rocm_make_buffer_kind_ops
//
// Given a buffer tracing 'kind' and a mapping of regular expression
// strings to gpu activity types, compute an array of operation
// indices that match and a map from op to gpu activities.
//
// input:
// kind: buffer tracing kind, eg, ROCPROFILER_BUFFER_TRACING_HIP_RUNTIME_API
// regex_input: map of regex strings to gpu activity major/minor
// regex_input_len: length of regex_input array
//
// optput:
// activity_vec: address of map from op index to major/minor pair
// activity_vec_len: length of activity_vec array
// ops_vec: address of operations vector for buffer tracing service
// ops_vec_len: length of ops_vec vector
//
// returns: 0 on success, -1 on failure.
//
// Notes:
// 1. "regular" means the POSIX regex(3) library
// 2. use '.*' for sequence of any characters (not '*')
// 3. match is case insensitive
// 4. don't need '.*' at begin/end of regex string
// 5. returned arrays are malloc()ed inside here
//
int
rocm_make_buffer_kind_ops
(
  rocprofiler_buffer_tracing_kind_t kind,
  rocm_regex_input_t regex_input[],
  int regex_input_len,
  gpu_activity_pair_t * activity_vec[],
  int * activity_vec_len,
  rocprofiler_tracing_operation_t *ops_vec[],
  int * ops_vec_len
)
{
  int num_ops, num_match;
  int i, op, ret;

  // Sanity test for invalid input.
  if (kind <= ROCPROFILER_BUFFER_TRACING_NONE
      || kind >= ROCPROFILER_BUFFER_TRACING_LAST)
  {
    EMSG("rocm_make_buffer_kind_ops: kind out of range: %d", kind);
    return FAILURE;
  }
  if (regex_input == NULL || regex_input_len <= 0) {
    EMSG("rocm_make_buffer_kind_ops: empty regex input");
    return FAILURE;
  }
  if (activity_vec == NULL || activity_vec_len == NULL) {
    EMSG("rocm_make_buffer_kind_ops: null activity_vec return vector");
    return FAILURE;
  }
  if (ops_vec == NULL || ops_vec_len == NULL) {
    EMSG("rocm_make_buffer_kind_ops: null ops_vec return vector");
    return FAILURE;
  }

  // Count the total number of ops for this kind.
  // We reach the end when operation_name() returns failure.
  num_ops = 0;
  for (op = 0; ; op++) {
    ret = rocm_get_buffer_kind_operation_name(kind, op, NULL, NULL);
    if (ret == ROCPROFILER_STATUS_SUCCESS) { num_ops++; }
    else { break; }
  }

  // Allocate and fill in array for ops names.
  // These are the strings that we match with the regex's.
  const char ** ops_name = malloc(num_ops * sizeof(char *));

  if (ops_name == NULL) {
    EMSG("rocm_make_buffer_kind_ops: malloc(ops name) failed, num_ops: %d",
         num_ops);
    return FAILURE;
  }
  for (op = 0; op < num_ops; op++) {
    ret = rocm_get_buffer_kind_operation_name(kind, op, &ops_name[op], NULL);
  }

  // Allocate array of regex_t and convert regex str to regex_t.
  regex_t * regex = calloc(regex_input_len, sizeof(regex_t));

  if (regex == NULL) {
    EMSG("rocm_make_buffer_kind_ops: malloc(regex array) failed, len: %d",
         regex_input_len);
    return FAILURE;
  }
  for (i = 0; i < regex_input_len; i++) {
    ret = regcomp(&regex[i], regex_input[i].str, REG_ICASE | REG_NOSUB);

    if (ret != 0) {
      EMSG("rocm_make_buffer_kind_ops: regcomp() failed");
      return FAILURE;
    }
  }

  // Allocate activity_vec return map from ops to activity types.
  *activity_vec = calloc(num_ops, sizeof(gpu_activity_pair_t));

  if (activity_vec == NULL) {
    EMSG("rocm_make_buffer_kind_ops: malloc(activity_vec) failed, num_ops: %d",
         num_ops);
    return FAILURE;
  }

  // Iterate through ops and regex's to see which match and save the
  // values in activity_vec.
  // List of regex's is inner so we can break on first match.
  num_match = 0;
  for (op = 0; op < num_ops; op++) {
    for (i = 0; i < regex_input_len; i++) {
      ret = regexec(&regex[i], ops_name[op], 0, NULL, 0);

      if (ret == 0) {
        (*activity_vec)[op].major = regex_input[i].major;
        (*activity_vec)[op].minor = regex_input[i].minor;
        num_match++;
        break;
      }
    }
  }

  if (ENABLED(ROCM)) {
    const char *kind_name = rocm_get_buffer_kind_name(kind);
    TMSG(ROCM, "rocm_make_buffer_kind_ops: kind(%d) %s (%d/%d)",
         kind, kind_name, num_match, num_ops);
  }

  // Assemble the list of matching ops in ops_vec.
  *ops_vec = malloc(num_match * sizeof(uint32_t));

  if (*ops_vec == NULL) {
    EMSG("rocm_make_buffer_kind_ops: malloc(return array) failed, num_match: %d",
         num_match);
    return FAILURE;
  }

  i = 0;
  for (op = 0; op < num_ops; op++) {
    if ((*activity_vec)[op].major != 0) {
      TMSG(ROCM, "op: %d  activity: (%d, %d)  %s",
           op, (*activity_vec)[op].major, (*activity_vec)[op].minor, ops_name[op]);

      (*ops_vec)[i] = op;
      i++;
      if (i >= num_match) {
        break;
      }
    }
  }

  // Free temp vars and return.
  free(ops_name);
  free(regex);

  *activity_vec_len = num_ops;
  *ops_vec_len = num_match;

  return 0;
}
