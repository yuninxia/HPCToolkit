// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0


//***************************************************************************

//
//  Dump the rocprofiler buffer tracing kind names and ops.  There are
//  14 'kinds' (rocprofiler_buffer_tracing_kind_t in fwd.h), and each
//  'kind' has a set of 'operations'.
//
//  Taken from samples: external_correlation_id_request/client.cpp,
//  fwd.h and various other kind-specific header files.
//
//  Compile me with:
//
//    ROCM = /opt/rocm    (or elsewhere)
//    gcc  -I${ROCM}/include  -L${ROCM}/lib  -lrocprofiler-sdk  \
//         kind-ops.c  -Wl,-rpath=${ROCM}/lib
//

//***************************************************************************

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#define  __HIP_PLATFORM_AMD__

#include <rocprofiler-sdk/rocprofiler.h>


int main(int argc, char ** argv)
{
    int kind, op, ret;

    // iterate over kinds
    for (kind = 1; kind < ROCPROFILER_BUFFER_TRACING_LAST; kind++) {
        int num_ops = 0;

        // count the total number of operations for this kind.
        // we reach the end when operation_name() returns failure.
        for (op = 0; ; op++) {
            ret = rocm_get_buffer_kind_operation_name(kind, op, NULL, NULL);

            if (ret == ROCPROFILER_STATUS_SUCCESS) { num_ops++; }
            else { break; }
        }

        const char *kind_name = rocm_get_buffer_kind_name(kind, &kind_name, NULL);

        // iterate over operations
        for (op = 0; ; op++) {
            const char * op_name = NULL;

            ret = rocm_get_buffer_kind_operation_name
              (kind, op, &op_name, NULL);

            if (ret == ROCPROFILER_STATUS_SUCCESS) {
                printf("%d   %s\n", op, op_name);
            }
            else {
                break;
            }
        }
    }
    printf("\n");

    return 0;
}
