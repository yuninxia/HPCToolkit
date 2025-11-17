// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*- // technically C99

#ifndef gpu_metric_h
#define gpu_metric_h

//*****************************************************************************
// local includes
//*****************************************************************************

#include "../../common/lean/gpu-metric-names.h"

#include "activity/gpu-activity.h"

//*****************************************************************************
// types
//*****************************************************************************


typedef enum {
  GPU_CYCLES_TYPE_ANY = 0
} gpu_cycles_all_t;

typedef enum {
  GPU_INST_STALL_ANY = 0
} gpu_inst_stall_all_t;

typedef enum {
  GPU_GMEM_LD_CACHED_BYTES = 0,
  GPU_GMEM_LD_UNCACHED_BYTES = 1,
  GPU_GMEM_ST_BYTES = 2,
  GPU_GMEM_LD_CACHED_L2TRANS = 3,
  GPU_GMEM_LD_UNCACHED_L2TRANS = 4,
  GPU_GMEM_ST_L2TRANS = 5,
  GPU_GMEM_LD_CACHED_L2TRANS_THEOR = 6,
  GPU_GMEM_LD_UNCACHED_L2TRANS_THEOR = 7,
  GPU_GMEM_ST_L2TRANS_THEOR = 8
} gpu_gmem_ops_t;

typedef enum {
  GPU_LMEM_LD_BYTES = 0,
  GPU_LMEM_ST_BYTES = 1,
  GPU_LMEM_LD_TRANS = 2,
  GPU_LMEM_ST_TRANS = 3,
  GPU_LMEM_LD_TRANS_THEOR = 4,
  GPU_LMEM_ST_TRANS_THEOR = 5
} gpu_lmem_ops_t;

typedef enum {
  GPAGE_MIG_SEC = 0,
  GPAGE_MIG_BYTES = 1,
  GPAGE_MIG_CNT = 2,
  GPAGE_FLT_SEC = 3,
  GPAGE_FLT_BYTES = 4,
  GPAGE_FLT_CNT = 5,
  GPAGE_QUEUE_CNT = 6,
  GPAGE_UNMAP_CNT = 7,
  GPAGE_UNMAP_BYTES = 8,
  GPAGE_DRP_CNT = 9
} gpu_page_metrics_t;

typedef enum {
  GPU_XFER_XMIT = 0,
  GPU_XFER_XRCV = 1,
  GPU_XFER_XMIT_TP = 2,
  GPU_XFER_XRCV_TP = 3,
  GPU_XFER_XMIT_COUNT = 4,
  GPU_XFER_XRCV_COUNT = 5
} gpu_xfer_ops_t;

typedef struct block_metrics_t {
  uint64_t execution_count;
  uint64_t latency;
  uint64_t active_simd_lanes;
} block_metrics_t;

typedef struct instruction_metrics_t {
  uint64_t execution_count;
  uint64_t latency;
  uint64_t total_simd_lanes;
  uint64_t active_simd_lanes;
  uint64_t wasted_simd_lanes;
  uint64_t scalar_simd_loss;
} instruction_metrics_t;



//--------------------------------------------------------------------------
// indexed metrics
//--------------------------------------------------------------------------

// gpu memory allocation/deallocation
#define FORALL_GMEM(macro)                                              \
  macro("GMEM:UNK (b)", GPU_MEM_UNKNOWN,                                \
        "GPU memory alloc/free: unknown memory kind (bytes)")           \
  macro("GMEM:PAG (b)", GPU_MEM_PAGEABLE,                               \
        "GPU memory alloc/free: pageable memory (bytes)")               \
  macro("GMEM:PIN (b)", GPU_MEM_PINNED,                                 \
        "GPU memory alloc/free: pinned memory (bytes)")                 \
  macro("GMEM:DEV (b)", GPU_MEM_DEVICE,                                 \
        "GPU memory alloc/free: device memory (bytes)")                 \
  macro("GMEM:ARY (b)", GPU_MEM_ARRAY,                                  \
        "GPU memory alloc/free: array memory (bytes)")                  \
  macro("GMEM:MAN (b)", GPU_MEM_MANAGED,                                \
        "GPU memory alloc/free: managed memory (bytes)")                \
  macro("GMEM:DST (b)", GPU_MEM_DEVICE_STATIC,                          \
        "GPU memory alloc/free: device static memory (bytes)")          \
  macro("GMEM:MST (b)", GPU_MEM_MANAGED_STATIC,                         \
        "GPU memory alloc/free: managed static memory (bytes)")         \
  macro("GMEM:COUNT", GPU_MEM_COUNT,                                    \
        "GPU memory alloc/free: count")

// gpu memory set
#define FORALL_GMSET(macro)                                             \
  macro("GMSET:UNK (b)", GPU_MEM_UNKNOWN,                               \
        "GPU memory set: unknown memory kind (bytes)")                  \
  macro("GMSET:PAG (b)", GPU_MEM_PAGEABLE,                              \
        "GPU memory set: pageable memory (bytes)")                      \
  macro("GMSET:PIN (b)", GPU_MEM_PINNED,                                \
        "GPU memory set: pinned memory (bytes)")                        \
  macro("GMSET:DEV (b)", GPU_MEM_DEVICE,                                \
        "GPU memory set: device memory (bytes)")                        \
  macro("GMSET:ARY (b)", GPU_MEM_ARRAY,                                 \
        "GPU memory set: array memory (bytes)")                         \
  macro("GMSET:MAN (b)", GPU_MEM_MANAGED,                               \
        "GPU memory set: managed memory (bytes)")                       \
  macro("GMSET:DST (b)", GPU_MEM_DEVICE_STATIC,                         \
        "GPU memory set: device static memory (bytes)")                 \
  macro("GMSET:MST (b)", GPU_MEM_MANAGED_STATIC,                        \
        "GPU memory set: managed static memory (bytes)")                \
  macro("GMSET:COUNT", GPU_MEM_COUNT,                                   \
        "GPU memory set: count")

// GPU cycles
#define FORALL_GPU_CYCLES_TYPE(macro)                                     \
  macro("GCYCLES", GPU_CYCLES_TYPE_ANY,                                   \
        "GPU cycles (estimated using PC sampling)")

// GPU instructions issued
#define FORALL_GPU_INST_TYPE(macro)                                       \
  macro("GCYCLES:ISU", GPU_INST_TYPE_ANY,                                 \
        "GPU issue cycles: issued any instruction")                       \
  macro("GCYCLES:ISU:MATR", GPU_INST_TYPE_MATRIX,                         \
        "GPU issue cycles: issued matrix instruction")                    \
  macro("GCYCLES:ISU:VEC2", GPU_INST_TYPE_VECTOR_DUAL,                    \
        "GPU issue cycles: issued dual vector instructions")              \
  macro("GCYCLES:ISU:VEC", GPU_INST_TYPE_VECTOR,                          \
        "GPU issue cycles: issued a vector instruction")                  \
  macro("GCYCLES:ISU:SCLR", GPU_INST_TYPE_SCALAR,                         \
        "GPU issue cycles: issued a scalar instruction")                  \
  macro("GCYCLES:ISU:TEX", GPU_INST_TYPE_TEXTURE,                         \
        "GPU issue cycles: issued a texture instruction")                 \
  macro("GCYCLES:ISU:LDS", GPU_INST_TYPE_LDS,                             \
        "GPU issue cycles: issued a Local Data Store instruction")        \
  macro("GCYCLES:ISU:LDSD", GPU_INST_TYPE_LDS_DIRECT,                     \
        "GPU issue cycles: issued a Local Data Store direct instruction") \
  macro("GCYCLES:ISU:FLAT", GPU_INST_TYPE_FLAT,                           \
        "GPU issue cycles: issued a flat instruction")                    \
  macro("GCYCLES:ISU:XPRT", GPU_INST_TYPE_EXPORT,                         \
        "GPU issue cycles: issued an export instruction")                 \
  macro("GCYCLES:ISU:MESG", GPU_INST_TYPE_MSG,                            \
        "GPU issue cycles: issued a message instruction")                 \
  macro("GCYCLES:ISU:BAR", GPU_INST_TYPE_BARRIER,                         \
        "GPU issue cycles: issued a barrier instruction")                 \
  macro("GCYCLES:ISU:BRT", GPU_INST_TYPE_BRANCH_TAKEN,                    \
        "GPU issue cycles: issued a branch taken instruction")            \
  macro("GCYCLES:ISU:BRNT", GPU_INST_TYPE_BRANCH_NOT_TAKEN,               \
        "GPU issue cycles: issued a branch not taken instruction")        \
  macro("GCYCLES:ISU:JMP", GPU_INST_TYPE_JUMP,                            \
        "GPU issue cycles: issued a jump instruction")                    \
  macro("GCYCLES:ISU:OTHR", GPU_INST_TYPE_OTHER,                          \
        "GPU issue cycles: issued an 'other' instruction")

#define FORALL_GPU_INST_STALL(macro)                                      \
  macro("GCYCLES:STL", GPU_INST_STALL_ANY,                                \
        "GPU exposed instruction issue stall cycles: any kind")           \
  macro("GCYCLES:STL:MEM", GPU_INST_STALL_MEM,                            \
        "GPU exposed instruction issue stall cycles: await completion of a kind of memory "   \
        "access")                                                         \
  macro("GCYCLES:STL:GMEM", GPU_INST_STALL_GMEM,                          \
        "GPU exposed instruction issue stall cycles: await completion of a global memory "      \
        "access")                                                         \
  macro("GCYCLES:STL:MTHR", GPU_INST_STALL_MEM_THROTTLE,                  \
        "GPU exposed instruction issue stall cycles: global memory request queue full")       \
  macro("GCYCLES:STL:TMEM", GPU_INST_STALL_TMEM,                          \
        "GPU exposed instruction issue stall cycles: texture memory request queue full")      \
  macro("GCYCLES:STL:CMEM", GPU_INST_STALL_CMEM,                          \
        "GPU exposed instruction issue stall cycles: await completion of constant or "        \
        "immediate memory access")                                        \
  macro("GCYCLES:STL:IFET", GPU_INST_STALL_IFETCH,                        \
        "GPU exposed instruction issue stall cycles: await availability of next "             \
        "instruction (fetch or branch delay)")                            \
  macro("GCYCLES:STL:IDEP", GPU_INST_STALL_IDEPEND,                       \
        "GPU exposed instruction issue stall cycles: await satisfaction of instruction "      \
        "input dependence")                                               \
  macro("GCYCLES:STL:PIPE", GPU_INST_STALL_PIPE_BUSY,                     \
        "GPU exposed instruction issue stall cycles: await completion of required "           \
        "compute resources")                                              \
  macro("GCYCLES:STL:SYNC", GPU_INST_STALL_SYNC,                          \
        "GPU exposed instruction issue stall cycles: await completion of thread or "          \
        "memory synchronization")                                         \
  macro("GCYCLES:STL:OTHR", GPU_INST_STALL_OTHER,                         \
        "GPU exposed instruction issue stall cycles: other")                                  \
  macro("GCYCLES:STL:SLP", GPU_INST_STALL_SLEEP,                          \
        "GPU exposed instruction issue stall cycles: sleep")                                  \
  macro("GCYCLES:STL:HID", GPU_INST_STALL_HIDDEN,                         \
        "GPU exposed instruction issue stall cycles: don't care because latency hidden")      \
  macro("GCYCLES:STL:INV", GPU_INST_STALL_INVALID,                        \
        "GPU exposed instruction issue stall cycles: invalid")

// GPU pipeline status: instantiate with either "STL" or "ISS"
#define FORALL_GPU_PIPE_TYPE(macro, si)                                 \
  macro(GPU_PIPE_METRIC_NAME ":" si ":MATR", GPU_PIPE_TYPE_MATRIX,      \
        "GPU pipeline " si " status: low precision matrix operation")   \
  macro(GPU_PIPE_METRIC_NAME ":" si ":VEC2", GPU_PIPE_TYPE_VECTOR_DUAL, \
        "GPU pipeline " si " status: vector, dual")                     \
  macro(GPU_PIPE_METRIC_NAME ":" si ":VEC", GPU_PIPE_TYPE_VECTOR,       \
        "GPU pipeline " si " status: vector")                           \
  macro(GPU_PIPE_METRIC_NAME ":" si ":SCLR", GPU_PIPE_TYPE_SCALAR,      \
        "GPU pipeline " si " status: scalar ALU or memory")             \
  macro(GPU_PIPE_METRIC_NAME ":" si ":LDS", GPU_PIPE_TYPE_LDS,          \
        "GPU pipeline " si " status: Local Data Store")                 \
  macro(GPU_PIPE_METRIC_NAME ":" si ":LDSD", GPU_PIPE_TYPE_LDS_DIRECT,  \
        "GPU pipeline " si " status: Local Data Store direct")          \
  macro(GPU_PIPE_METRIC_NAME ":" si ":TEX", GPU_PIPE_TYPE_TEXTURE,      \
        "GPU pipeline " si " status: texture")                          \
  macro(GPU_PIPE_METRIC_NAME ":" si ":FLAT", GPU_PIPE_TYPE_FLAT,        \
        "GPU pipeline " si " status: flat")                             \
  macro(GPU_PIPE_METRIC_NAME ":" si ":XPRT", GPU_PIPE_TYPE_EXPORT,      \
        "GPU pipeline " si " status: export")                           \
  macro(GPU_PIPE_METRIC_NAME ":" si ":BMSG", GPU_PIPE_TYPE_BRMSG,       \
        "GPU pipeline " si " status: branch or message")                \
  macro(GPU_PIPE_METRIC_NAME ":" si ":MISC", GPU_PIPE_TYPE_MISC,        \
        "GPU pipeline " si " status: miscellaneous")

#define FORALL_GPU_PIPE_TYPE_ISS(macro) FORALL_GPU_PIPE_TYPE(macro, "ISU")
#define FORALL_GPU_PIPE_TYPE_STL(macro) FORALL_GPU_PIPE_TYPE(macro, "STL")


#define FORALL_GPU_UTIL_METRICS(macro)                                  \
  macro("GCYCLES:WAVE_ACT", GPU_UTIL_METRICS_WAVE_ACT,                   \
        "GPU waves aggregate active")                                   \
  macro("GCYCLES:WAVE_AVL", GPU_UTIL_METRICS_WAVE_AVL,                  \
        "GPU waves aggregate available")                                \
  macro("GCYCLES:WAVE_UTL", GPU_UTIL_METRICS_WAVE_UTL,                  \
        "GPU waves utilization (actual occupancy): "                    \
        "100*(waves active)/(waves available)")                         \
  macro("GCYCLES:THR_ACT", GPU_UTIL_METRICS_THR_ACT,                    \
        "GPU SIMD lanes (threads) aggregate active")                    \
  macro("GCYCLES:THR_AVL", GPU_UTIL_METRICS_THR_AVL,                    \
        "GPU SIMD lanes (threads) aggregate available ")                \
  macro("GCYCLES:THR_UTL", GPU_UTIL_METRICS_THR_UTL,                    \
        "GPU SIMD lanes (threads) utilization: 100*(SIMD lanes active)/(SIMD lanes available)")

// gpu explicit copy
#define FORALL_GXCOPY(macro)                                            \
  macro("GXCOPY:UNK (b)", GPU_MEMCPY_UNK,                               \
        "GPU explicit memory copy: unknown kind (bytes)")               \
  macro("GXCOPY:H2D (b)", GPU_MEMCPY_H2D,                               \
        "GPU explicit memory copy: host to device (bytes)")             \
  macro("GXCOPY:D2H (b)", GPU_MEMCPY_D2H,                               \
        "GPU explicit memory copy: device to host (bytes)")             \
  macro("GXCOPY:H2A (b)", GPU_MEMCPY_H2A,                               \
        "GPU explicit memory copy: host to array (bytes)")              \
  macro("GXCOPY:A2H (b)", GPU_MEMCPY_A2H,                               \
        "GPU explicit memory copy: array to host (bytes)")              \
  macro("GXCOPY:A2A (b)", GPU_MEMCPY_A2A,                               \
        "GPU explicit memory copy: array to array (bytes)")             \
  macro("GXCOPY:A2D (b)", GPU_MEMCPY_A2D,                               \
        "GPU explicit memory copy: array to device (bytes)")            \
  macro("GXCOPY:D2A (b)", GPU_MEMCPY_D2A,                               \
        "GPU explicit memory copy: device to array (bytes)")            \
  macro("GXCOPY:D2D (b)", GPU_MEMCPY_D2D,                               \
        "GPU explicit memory copy: device to device (bytes)")           \
  macro("GXCOPY:H2H (b)", GPU_MEMCPY_H2H,                               \
        "GPU explicit memory copy: host to host (bytes)")               \
  macro("GXCOPY:P2P (b)", GPU_MEMCPY_P2P,                               \
        "GPU explicit memory copy: peer to peer (bytes)")               \
  macro("GXCOPY:COUNT", GPU_MEMCPY_COUNT,                               \
        "GPU explicit memory copy: count")

#define FORALL_GSYNC(macro)                                             \
  macro("GSYNC:UNK (s)", GPU_SYNC_UNKNOWN,                              \
        "GPU synchronizations: unknown kind (seconds)")                 \
  macro("GSYNC:EVT (s)", GPU_SYNC_EVENT,                                \
        "GPU synchronizations: event (seconds)")                        \
  macro("GSYNC:STRE (s)", GPU_SYNC_STREAM_EVENT_WAIT,                   \
        "GPU synchronizations: stream event wait (seconds)")            \
  macro("GSYNC:STR (s)", GPU_SYNC_STREAM,                               \
        "GPU synchronizations: stream (seconds)")                       \
  macro("GSYNC:CTX (s)", GPU_SYNC_CONTEXT,                              \
        "GPU synchronizations: context (seconds)")                      \
  macro("GSYNC:COUNT", GPU_SYNC_COUNT,                                  \
        "GPU synchronizations: count (seconds)")

#define FORALL_GPAGE(macro)                                             \
  macro("GPAGE:MIG (s)", GPAGE_MIG_SEC,                                 \
        "GPU runtime paging: migration time (seconds)")                 \
  macro("GPAGE:FLT (s)", GPAGE_FLT_SEC,                                 \
        "GPU runtime paging: page fault time (seconds)")                \
                                                                        \
  macro("GPAGE:MIGSZ (b)", GPAGE_MIG_BYTES,                             \
        "GPU runtime paging: migration bytes")                          \
  macro("GPAGE:FLTSZ (b)", GPAGE_FLT_BYTES,                             \
        "GPU runtime paging: page fault bytes")                         \
  macro("GPAGE:UMPSZ (b)", GPAGE_UNMAP_BYTES,                           \
        "GPU runtime paging: unmap")                                    \
                                                                        \
  macro("GPAGE:MIGCNT", GPAGE_MIG_CNT,                                  \
        "GPU runtime paging: migration count")                          \
  macro("GPAGE:FLTCNT", GPAGE_FLT_CNT,                                  \
        "GPU runtime paging: page fault count")                         \
  macro("GPAGE:QSVCNT", GPAGE_QSV_CNT,                                  \
        "GPU runtime paging: queue eviction count")                     \
  macro("GPAGE:UMPCNT (bytes)", GPAGE_UNMAP_CNT,                        \
        "GPU runtime paging: unmap")                                    \
  macro("GPAGE:DRPCNT", GPAGE_DRP_CNT,                                  \
        "GPU runtime paging: dropped records count")

#define FORALL_GSYNC(macro)                                             \
  macro("GSYNC:UNK (s)", GPU_SYNC_UNKNOWN,                              \
        "GPU synchronizations: unknown kind (seconds)")                 \
  macro("GSYNC:EVT (s)", GPU_SYNC_EVENT,                                \
        "GPU synchronizations: event (seconds)")                        \
  macro("GSYNC:STRE (s)", GPU_SYNC_STREAM_EVENT_WAIT,                   \
        "GPU synchronizations: stream event wait (seconds)")            \
  macro("GSYNC:STR (s)", GPU_SYNC_STREAM,                               \
        "GPU synchronizations: stream (seconds)")                       \
  macro("GSYNC:CTX (s)", GPU_SYNC_CONTEXT,                              \
        "GPU synchronizations: context (seconds)")                      \
  macro("GSYNC:COUNT", GPU_SYNC_COUNT,                                  \
        "GPU synchronizations: count (seconds)")

#define FORALL_GSCR(macro)                                              \
  macro("GSCR (s)", GSCR_SEC,                                           \
        "GPU scratch operation time (seconds)")

// gpu global memory access
#define FORALL_GGMEM(macro)                                             \
  macro("GGMEM:LDC (b)", GPU_GMEM_LD_CACHED_BYTES,                      \
        "GPU global memory: load cacheable memory (bytes)")             \
  macro("GGMEM:LDU (b)", GPU_GMEM_LD_UNCACHED_BYTES,                    \
        "GPU global memory: load uncacheable memory (bytes)")           \
  macro("GGMEM:ST (b)", GPU_GMEM_ST_BYTES,                              \
        "GPU global memory: store (bytes)")                             \
                                                                        \
  macro("GGMEM:LDC (L2T)", GPU_GMEM_LD_CACHED_L2TRANS,                  \
        "GPU global memory: load cacheable (L2 cache transactions)")    \
  macro("GGMEM:LDU (L2T)", GPU_GMEM_LD_UNCACHED_L2TRANS,                \
        "GPU global memory: load uncacheable (L2 cache transactions)")  \
  macro("GGMEM:ST (L2T)", GPU_GMEM_ST_L2TRANS,                          \
        "GPU global memory: store (L2 cache transactions)")             \
                                                                        \
  macro("GGMEM:LDCT (L2T)", GPU_GMEM_LD_CACHED_L2TRANS_THEOR,           \
        "GPU global memory: load cacheable "                            \
        "(L2 cache transactions, theoretical)")                         \
  macro("GGMEM:LDUT (L2T)", GPU_GMEM_LD_UNCACHED_L2TRANS_THEOR,         \
        "GPU global memory: load uncacheable "                          \
        "(L2 cache transactions, theoretical)")                         \
  macro("GGMEM:STT (L2T)", GPU_GMEM_ST_L2TRANS_THEOR,                   \
        "GPU global memory: store "                                     \
        "(L2 cache transactions, theoretical)")

// gpu local memory access
#define FORALL_GLMEM(macro)                                             \
  macro("GLMEM:LD (b)", GPU_LMEM_LD_BYTES,                              \
        "GPU local memory: load (bytes)")                               \
  macro("GLMEM:ST (b)", GPU_LMEM_ST_BYTES,                              \
        "GPU local memory: store (bytes)")                              \
                                                                        \
  macro("GLMEM:LD (T)", GPU_LMEM_LD_TRANS,                              \
        "GPU local memory: load (transactions)")                        \
  macro("GLMEM:ST (T)", GPU_LMEM_ST_TRANS,                              \
        "GPU local memory: store (transactions)")                       \
                                                                        \
  macro("GLMEM:LDT (T)", GPU_LMEM_LD_TRANS_THEOR,                       \
        "GPU local memory: load (transactions, theoretical)")           \
  macro("GLMEM:STT (T)", GPU_LMEM_ST_TRANS_THEOR,                       \
        "GPU local memory: store (transactions, theoretical)")

//--------------------------------------------------------------------------
// scalar metrics
//--------------------------------------------------------------------------

// gpu activity times
#define FORALL_GTIMES(macro)                                            \
  macro("GPUOP (s)", GPU_TIME_OP,                                       \
        "GPU time: all operations (seconds)")                           \
  macro("GKER (s)", GPU_TIME_KER,                                       \
        "GPU time: kernel execution (seconds)")                         \
  macro("GMEM (s)", GPU_TIME_MEM,                                       \
        "GPU time: memory allocation/deallocation (seconds)")           \
  macro("GMSET (s)", GPU_TIME_MSET,                                     \
        "GPU time: memory set (seconds)")                               \
  macro("GXCOPY (s)", GPU_TIME_XCOPY,                                   \
        "GPU time: explicit data copy (seconds)")                       \
  macro("GICOPY (s)", GPU_TIME_ICOPY,                                   \
        "GPU time: implicit data copy (seconds)")                       \
  macro("GSYNC (s)", GPU_TIME_SYNC,                                     \
        "GPU time: synchronization (seconds)")

// gpu instruction count
#define FORALL_GPU_INST(macro)                                          \
  macro(GPU_INST_METRIC_NAME ":BLK_EXEC_CNT", GPU_BLOCK_EXECUTION_COUNT,\
        "GPU basic block execution count")                              \
  macro(GPU_INST_METRIC_NAME ":BLK_LAT", GPU_BLOCK_LATENCY,             \
        "GPU basic block latency")                                      \
  macro(GPU_INST_METRIC_NAME ":BLK_SIMD_ACT", GPU_BLOCK_ACTIVE_SIMD_LANES, \
        "GPU basic block active SIMD lanes")                            \
  macro(GPU_INST_METRIC_NAME, GPU_INSTRUCTION_EXECUTION_COUNT,          \
        "GPU instruction execution count")                              \
  macro(GPU_INST_METRIC_NAME ":LAT", GPU_INSTRUCTION_LATENCY,           \
        "GPU instruction latency")                                      \
  macro(GPU_INST_METRIC_NAME ":LAT_COV", GPU_INSTRUCTION_COVERED_LATENCY, \
        "GPU instruction covered latency")                              \
  macro(GPU_INST_METRIC_NAME ":LAT_UCV", GPU_INSTRUCTION_UNCOVERED_LATENCY, \
        "GPU instruction uncovered latency")                            \
  macro(GPU_INST_METRIC_NAME ":LAT_THR", GPU_INSTRUCTION_THREADS_TO_COVER_LATENCY, \
        "GPU threads needed to cover latency (1 + UCV/COV)")            \
  macro(GPU_INST_METRIC_NAME ":SIMD_TOT", GPU_INSTRUCTION_TOTAL_SIMD_LANES, \
        "GPU instruction total SIMD lanes")                             \
  macro(GPU_INST_METRIC_NAME ":SIMD_ACT", GPU_INSTRUCTION_ACTIVE_SIMD_LANES, \
        "GPU instruction active SIMD lanes")                            \
  macro(GPU_INST_METRIC_NAME ":SIMD_WST", GPU_INSTRUCTION_WASTED_SIMD_LANES, \
        "GPU instruction wasted SIMD lanes")                            \
  macro(GPU_INST_METRIC_NAME ":SIMD_SLS", GPU_INSTRUCTION_SCALAR_SIMD_LOSS, \
        "GPU instruction SIMD lanes lost due to scalar instructions")

// gpu kernel characteristics
#define FORALL_KINFO(macro)                                                                                                  \
  macro("GKER:STMEM_ACUMU (b)", GPU_KINFO_STMEM_ACUMU,                  \
        "GPU kernel: static memory accumulator [internal use only]")    \
  macro("GKER:DYMEM_ACUMU (b)", GPU_KINFO_DYMEM_ACUMU,                  \
        "GPU kernel: dynamic memory accumulator [internal use only]")   \
  macro("GKER:LMEM_ACUMU (b)", GPU_KINFO_LMEM_ACUMU,                    \
        "GPU kernel: local memory accumulator [internal use only]")     \
  macro("GKER:FGP_ACT_ACUMU", GPU_KINFO_FGP_ACT_ACUMU,                  \
        "GPU kernel: fine-grain parallelism accumulator [internal use only]") \
  macro("GKER:FGP_MAX_ACUMU", GPU_KINFO_FGP_MAX_ACUMU,                  \
        "GPU kernel: fine-grain parallelism accumulator [internal use only]") \
  macro("GKER:THR_SREG_ACUMU", GPU_KINFO_SREG_ACUMU,                    \
        "GPU kernel: scalar register count accumulator [internal use only]") \
  macro("GKER:THR_VREG_ACUMU", GPU_KINFO_VREG_ACUMU,                    \
        "GPU kernel: vector register count accumulator [internal use only]") \
  macro("GKER:BLK_THR_ACUMU", GPU_KINFO_BLK_THREADS_ACUMU,              \
        "GPU kernel: thread count accumulator [internal use only]")     \
  macro("GKER:BLK_SM_ACUMU", GPU_KINFO_BLK_SMEM_ACUMU,                  \
        "GPU kernel: block local memory accumulator [internal use only]") \
  macro("GKER:BLKS_ACUMU", GPU_KINFO_BLKS_ACUMU,                        \
        "GPU kernel: block count accumulator [internal use only]")      \
  macro("GKER:STMEM (b)", GPU_KINFO_STMEM,                              \
        "GPU kernel: static memory (bytes)")                            \
  macro("GKER:DYMEM (b)", GPU_KINFO_DYMEM,                              \
        "GPU kernel: dynamic memory (bytes)")                           \
  macro("GKER:LMEM (b)", GPU_KINFO_LMEM,                                \
        "GPU kernel: local memory (bytes)")                             \
  macro("GKER:FGP_ACT", GPU_KINFO_FGP_ACT,                              \
        "GPU kernel: fine-grain parallelism, actual")                   \
  macro("GKER:FGP_MAX", GPU_KINFO_FGP_MAX,                              \
        "GPU kernel: fine-grain parallelism, maximum")                  \
  macro("GKER:SREG", GPU_KINFO_SREG,                                    \
        "GPU kernel: scalar register count")                            \
  macro("GKER:VREG", GPU_KINFO_VREG,                                    \
        "GPU kernel: vector register count")                            \
  macro("GKER:BLK_THR", GPU_KINFO_BLK_THREADS,                          \
        "GPU kernel: thread count")                                     \
  macro("GKER:BLK_SM (b)", GPU_KINFO_BLK_SMEM,                          \
        "GPU kernel: block local memory (bytes)")                       \
  macro("GKER:BLKS", GPU_KINFO_BLKS,                                    \
        "GPU kernel: block count")                                      \
  macro("GKER:COUNT", GPU_KINFO_COUNT,                                  \
        "GPU kernel: launch count")                                     \
  macro("GKER:OCC_THR", GPU_KINFO_OCCUPANCY_THR,                        \
        "GPU kernel: theoretical occupancy (FGP_ACT / FGP_MAX)")

// gpu implicit copy
#define FORALL_GICOPY(macro)                                            \
  macro("GICOPY:UNK (b)", GPU_ICOPY_UNKNOWN,                            \
        "GPU implicit copy: unknown kind (bytes)")                      \
  macro("GICOPY:H2D (b)", GPU_ICOPY_H2D_BYTES,                          \
        "GPU implicit copy: host to device (bytes)")                    \
  macro("GICOPY:D2H (b)", GPU_ICOPY_D2H_BYTES,                          \
        "GPU implicit copy: device to host (bytes)")                    \
  macro("GICOPY:D2D (b)", GPU_ICOPY_D2D_BYTES,                          \
        "GPU implicit copy: device to device (bytes)")                  \
  macro("GICOPY:CPU_PF", GPU_ICOPY_CPU_PF,                              \
        "GPU implicit copy: CPU page faults")                           \
  macro("GICOPY:GPU_PF", GPU_ICOPY_GPU_PF,                              \
        "GPU implicit copy: GPU page faults")                           \
  macro("GICOPY:THRASH", GPU_ICOPY_THRASH,                              \
        "GPU implicit copy: CPU thrashing page faults "                 \
        "(data frequently migrating between processors)")               \
  macro("GICOPY:THROT", GPU_ICOPY_THROT,                                \
        "GPU implicit copy: throttling "                                \
        "(prevent thrashing by delaying page fault service)")           \
  macro("GICOPY:RMAP", GPU_ICOPY_RMAP,                                  \
        "GPU implicit copy: remote maps "                               \
        "(prevent thrashing by pinning memory for a time with "         \
        "some processor mapping and accessing it remotely)")            \
  macro("GICOPY:COUNT", GPU_ICOPY_COUNT,                                \
        "GPU implicit copy: count")

// gpu branch
#define FORALL_GBR(macro)               \
  macro("GBR:DIV", GPU_BR_DIVERGED,     \
        "GPU branches: diverged")       \
  macro("GBR:EXE", GPU_BR_EXECUTED,     \
        "GPU branches: executed")

// gpu sampling information
#define FORALL_GSAMP_INT(macro)                         \
  macro("GSAMP:DRP", GPU_SAMPLE_DROPPED,                \
        "GPU PC samples: dropped")                      \
  macro("GSAMP:EXP", GPU_SAMPLE_EXPECTED,               \
        "GPU PC samples: expected")                     \
  macro("GSAMP:TOT", GPU_SAMPLE_TOTAL,                  \
        "GPU PC samples: measured")                     \
  macro("GSAMP:PER (cyc)", GPU_SAMPLE_PERIOD,           \
        "GPU PC samples: period (GPU cycles)")

#define FORALL_GSAMP_REAL(macro)                        \
  macro("GSAMP:UTIL", GPU_SAMPLE_UTILIZATION,           \
        "GPU utilization computed using PC sampling")

#define FORALL_GSAMP(macro) \
  FORALL_GSAMP_INT(macro)   \
  FORALL_GSAMP_REAL(macro)

// gpu transfer information
#define FORALL_GXFER(macro)                                             \
  macro("GXFER:XMIT (b)", GPU_XFER_XMIT,                                \
        "GPU link total data transmitted")                              \
  macro("GXFER:XRCV (b)", GPU_XFER_XRCV,                                \
        "GPU link total data received")                                 \
  macro("GXFER:XMIT_TP (GB)", GPU_XFER_XMIT_TP,                         \
        "GPU link total transmit throughput")                           \
  macro("GXFER:XRCV_TP (GB)", GPU_XFER_XRCV_TP,                         \
        "GPU link total received throughput")                           \
  macro("GXFER:XMIT_COUNT", GPU_XFER_XMIT_COUNT,                        \
        "GPU link launch count transmitted")                            \
  macro("GXFER:XRCV_COUNT", GPU_XFER_XRCV_COUNT,                        \
        "GPU kernel: launch count received")

// intel optimization metrics
#define FORALL_INTEL_OPTIMIZATION(macro)                                                                                                   \
  macro("INORDER_QUEUE:COUNT", INORDER_QUEUE,                           \
        "count of inorder GPU queues/streams (enable out-of-order execution to run kernels in parallel)") \
  macro("GKER_MULTIPLE_CONTEXTS:COUNT", KERNEL_TO_MULTIPLE_CONTEXTS,    \
        "count of kernel executions on multiple contexts (each context will JIT the kernel)") \
  macro("GKER_PARAMS_NOT_ALIASED:COUNT", KERNEL_PARAMS_NOT_ALIASED,     \
        "count of kernel invocations with non-aliased parameters (add directive for enabling code-reordering optimization)") \
  macro("GKER_PARAMS_ALIASED:COUNT", KERNEL_PARAMS_ALIASED,             \
        "count of kernel invocations with aliased parameters")          \
  macro("SINGLE_DEVICE_USE_AOT_COMPILATION", SINGLE_DEVICE_USE_AOT_COMPILATION, \
        "since a single device is being used, use AOT for saving time JITing kernels: bool") \
  macro("OUTPUT_OF_KERNEL_INPUT_TO_ANOTHER_KERNEL", OUTPUT_OF_KERNEL_INPUT_TO_ANOTHER_KERNEL, \
        "kernel output is input for another kernel. Try merging kernels to avoid sending redundant data to GPU: bool") \
  macro("UNUSED_DEVICES:COUNT", UNUSED_DEVICES,                         \
        "all available devices are not getting utilized. Offload computations to all devices to reduce total application execution time: count")

// blame-shifting metrics
#define FORALL_BLAME_SHIFT(macro)                                       \
  macro("CPU_IDLE (s)", CPU_IDLE,                                       \
        "CPU_IDLE time (seconds)")                                      \
  macro("GPU_IDLE_CAUSE (s)", GPU_IDLE_CAUSE,                           \
        "GPU_IDLE_CAUSE time (seconds)")                                \
  macro("CPU_IDLE_CAUSE (s)", CPU_IDLE_CAUSE,                           \
        "CPU_IDLE_CAUSE time (seconds)")

// gpu-utilization metrics
#define FORALL_GPU_UTILIZATION(macro)                                   \
  macro("EU_ACTIVE", EU_ACT, "")                                        \
  macro("EU_STALL", EU_STL, "")                                         \
  macro("EU_IDLE", EU_IDLE, "")                                         \
  macro("GPU_UTIL_DENOMINATOR", GPU_UTIL_DENOMINATOR,                   \
        "this is a helper metric that increments the metric value by 100 for the corresponding CCT. This can be denominator to the above three \
  to metrics to get the \% of GPU utilization")                         \
  macro("EU_ACT (%)", EU_ACT_PERCENT,                                   \
        "The percentage of time in which the Execution Units were active") \
  macro("EU_STL (%)", EU_STL_PERCENT,                                   \
        "The percentage of time in which the Execution Units were stalled") \
  macro("EU_IDLE (%)", EU_IDLE_PERCENT,                                 \
        "The percentage of time in which the Execution Units were idle")

//******************************************************************************
// interface operations
//******************************************************************************

//--------------------------------------------------
// enable default GPU metrics
//--------------------------------------------------

void gpu_metrics_default_enable
(
 void
);

//--------------------------------------------------
// record NVIDIA kernel info
//--------------------------------------------------

void gpu_metrics_KINFO_enable(
    void);

//--------------------------------------------------
// record implicit copy metrics for unified memory
//--------------------------------------------------

void gpu_metrics_GICOPY_enable(
    void);

//----------------------------------------
// instruction sampling metrics
//----------------------------------------

// record gpu instruction sample counts
void
gpu_metrics_GPU_INST_enable
(
 void
);

// record NVIDIA GPU instruction stall reasons
void gpu_metrics_GPU_INST_STALL_enable
(
 void
);

// record NVIDIA GPU instruction sampling statistics
void gpu_metrics_GSAMP_enable
(
 void
);

void gpu_metrics_GXFER_enable
(
 void
);

// record blame-shifting metrics
void gpu_metrics_BLAME_SHIFT_enable
(
 void
);

void gpu_metrics_gpu_utilization_enable
(
 void
);

//--------------------------------------------------
// record global memory access statistics
//--------------------------------------------------

void gpu_metrics_GGMEM_enable
(
 void
);

//--------------------------------------------------
// record local memory access statistics
//--------------------------------------------------

void gpu_metrics_GLMEM_enable
(
 void
);

//--------------------------------------------------
// record branch statistics
//--------------------------------------------------

void gpu_metrics_GBR_enable
(
 void
);


//--------------------------------------------------
// record GPU PC sampling cycles
//--------------------------------------------------

void
gpu_metrics_GPU_CYCLES_TYPE_enable
(
  void
);


//--------------------------------------------------
// record instruction issue statistics
//--------------------------------------------------

void
gpu_metrics_GPU_INST_TYPE_enable
(
  void
);

//--------------------------------------------------
// record pipeline issue/stall statistics
//--------------------------------------------------

void
gpu_metrics_GPU_PIPE_enable
(
  void
);


//--------------------------------------------------
// record GPU inst-level wave and SIMD utilization
//--------------------------------------------------

void
gpu_metrics_GPU_UTIL_METRICS_enable
(
  void
);


//--------------------------------------------------
// record GPU hardware counters
//--------------------------------------------------

// Unlike other GPU metric types that may have up to a dozen of metrics,
// GPU hardware counters may have a few hundred metrics.
// So, we should only create counter metrics for the ones that are
// requested at the command line.
//
// returns metric_id of first GPU metric
int gpu_metrics_GPU_CTR_enable
(
 int,
 const char **,
 const char **
);

//--------------------------------------------------
// intel optimization metrics
//--------------------------------------------------

void gpu_metrics_INTEL_OPTIMIZATION_enable
(
 void
);

//--------------------------------------------------
// attribute GPU measurements to an application
// thread's calling context tree
//--------------------------------------------------

void gpu_metrics_attribute
(
 gpu_activity_t *activity
);

block_metrics_t
fetch_block_metrics
(
 cct_node_t *
);

void
attribute_instruction_metrics
(
 cct_node_t *,
 instruction_metrics_t
);

#endif
