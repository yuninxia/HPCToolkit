<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

(chpt:gpu)=

# Monitoring GPU Operations

HPCToolkit can measure both CPU and GPU performance of GPU-accelerated applications. It measures CPU performance using asynchronous sampling, as described in
Section [6.3](#sample-sources), and it measures GPU operations using tool libraries or interfaces provided by GPU vendors.
A single build of HPCToolkit can support GPUs from AMD, Intel, and NVIDIA.

In this chapter, we describe HPCToolkit's capabilities for profiling, tracing, and PC sampling of GPU-accelerated applications that use CUDA, ROCm, Level Zero, OpenCL, and/or OpenMP offloading.

Section [9.1](#sec:gpu-quickstart) provides a quick start guide for using HPCToolkit to measure GPU-accelerated applications.
Subsequent sections describe profiling (Section [9.2](#sec:gpu-profiling)), tracing (Section [9.3](#sec:gpu-tracing)), PC sampling (Section [9.4](#sec:gpu-sampling)), and associated platform-independent metrics.
The chapter concludes with sections on CUDA (Section [9.5](#sec:gpu-cuda)), ROCm (Section [9.6](#sec:gpu-rocm)), and Level Zero (Section [9.7](#sec:gpu-level0)) that discuss platform-specific measurement details or metrics.

(sec:gpu-quickstart)=

## GPU Measurement Quickstart

HPCToolkit provides a vendor-independent monitoring substrate for measuring the performance of GPU-accelerated applications.
This substrate interfaces with NVIDIA's [CUPTI](https://docs.nvidia.com/cupti) (CUDA Performance Tools Interface) and AMD's [Rocprofiler-sdk](https://rocm.docs.amd.com/projects/rocprofiler-sdk). It also supports intercepting calls to the OpenCL API and Intel's Level Zero API because OpenCL and Level Zero lack a measurement API like CUPTI or Rocprofiler-sdk.

For each of the supported GPU runtimes (ROCm, CUDA, Level Zero, and OpenCL), HPCToolkit supports several monitoring options. All of the GPU runtimes support profiling and tracing of GPU operations. ROCm, CUDA, and Level Zero support instruction-level measurement within GPU kernels using PC sampling. Level Zero also supports instruction-level performance measurement within GPU kernels using binary instrumentation with Intel's GTPin.

Table [9.1](#amd-options) shows arguments to `hpcrun` that can be used to monitor the performance of GPU operations offloaded by HIP or OpenMP on AMD GPUs. Note that there are two types of PC sampling: hardware (`hw`) and software-only (`sw`). `sw` PC sampling is the default mode because it is available on MI200+; `hw` PC sampling is available only on MI300+. For `sw` PC sampling, the default period is 2^17 ns. For `hw` PC sampling, the default period is 2^20 cycles.

```{table} Table 9.1: Monitoring performance on AMD GPUs when using AMD's HIP and OpenMP programming models atop AMD's ROCm runtime.
---
name: amd-options
widths: grid
---
| Argument to `hpcrun`            | What is monitored                                                                               |
| :------------------------------ | :---------------------------------------------------------------------------------------------- |
| `-e gpu=rocm`                   | coarse-grain profiling of AMD GPU operations                                                    |
| `-e gpu=rocm -t`                | coarse-grain profiling and tracing of AMD GPU operations                                        |
| `-e gpu=rocm -tt`               | coarse-grain profiling and high-resolution tracing of AMD GPU operations                        |
| `-e gpu=rocm,pc[={sw,hw}][@k]`  | coarse-grain profiling of GPU operations; fine-grain profiling within GPU kernels using PC sampling every 2^k cycles (hw) or 2^k ns (sw). |
```

Table [9.2](#nvidia-cuda-monitoring-options) shows arguments to `hpcrun` that can be used to monitor the performance of GPU operations offloaded by CUDA or OpenMP on NVIDIA GPUs.
On NVIDIA GPUs, the default period for PC sampling is 2^12 cycles.

```{table} Table 9.2: Monitoring performance on NVIDIA GPUs when using CUDA or OpenMP programming models.
---
name: nvidia-cuda-monitoring-options
---
| Argument to `hpcrun` | What is monitored                                                                                   |
| :--------------------| :-------------------------------------------------------------------------------------------------- |
| `-e gpu=cuda`        | coarse-grain profiling of GPU operations                                                            |
| `-e gpu=cuda -t`     | coarse-grain profiling and tracing of GPU operations                                                |
| `-e gpu=cuda -tt`    | coarse-grain profiling and high-resolution tracing of GPU operations                                |
| `-e gpu=cuda,pc[@k]` | coarse-grain profiling of GPU operations; fine-grain profiling within GPU kernels using PC sampling every 2^k cycles |
```

Table [9.3](#intel-level0-options) shows available options for using HPCToolkit that can be used to monitor the performance of GPU operations offloaded by SYCL or OpenMP to Intel's Level Zero runtime. On Intel GPUs, the default period for PC sampling is 2^19 ns.

```{table} Table 9.3: Monitoring performance on Intel GPUs when using SYCL or OpenMP programming models atop Intel's Level Zero runtime.
---
name: intel-level0-options
---
| Argument to `hpcrun`       | What is monitored                                                                                                                      |
| :------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------|
| `-e gpu=level0`            | coarse-grain profiling of GPU operations runtime                                                                                       |
| `-e gpu=level0 -t`         | coarse-grain profiling and tracing of GPU operations                                                                                   |
| `-e gpu=level0 -tt`        | coarse-grain profiling and high-resolution tracing of GPU operations                                                                   |
| `-e gpu=level0,inst=count` | coarse-grain profiling of GPU operations; fine-grain measurement within kernels using binary instrumentation to count instructions     |
| `-e gpu=level0,pc[@k]`     | coarse-grain profiling of GPU operations; fine-grain profiling within GPU kernels using PC sampling every 2^k ns                       |
```

Table [9.4](#opencl-monitoring-options) shows the possible command-line arguments to `hpcrun` for monitoring OpenCL programs.

```{table} Table 9.4: Monitoring performance on GPUs when using the OpenCL programming model.
---
name: opencl-monitoring-options
---
| Argument to `hpcrun` | What is monitored                                                                      |
| :------------------- | :------------------------------------------------------------------------------------- |
| `-e gpu=opencl`      | coarse-grain profiling of GPU operations using a platform's OpenCL runtime             |
| `-e gpu=opencl -t`   | coarse-grain profiling and tracing of GPU operations using a platform's OpenCL runtime |
| `-e gpu=opencl -tt`  | coarse-grain profiling and high-resolution tracing of GPU operations using a platform's OpenCL runtime                 |
```

(sec:gpu-profiling)=

## Profiling GPU Operations

In most cases, HPCToolkit reports performance metrics for GPU operations in a vendor-neutral way. For instance, rather than focusing on NVIDIA GPU warps or AMD GPU wavefronts, HPCToolkit presents both as fine-grain, thread-level parallelism.

Coarse-grain profiling attributes to each calling context the total time of all GPU operations initiated in that context. Table [9.5](#table:gtimes) shows the classes of GPU operations for which timings are collected. For AMD and NVIDIA GPUs, HPCToolkit also reports GPU kernel characteristics, including including register usage, thread count per block, and theoretical occupancy as shown in Table [9.6](#table:gker). HPCToolkit derives a theoretical GPU occupancy metric as the ratio of the actual number of threads in a block to be scheduled on an AMD compute unit or an NVIDIA streaming multiprocessor to the maximum number of threads supported in a block.

In addition, HPCToolkit records metrics for operations performed including memory allocation and deallocation (Table [9.7](#table:gmem)), memory set (Table [9.8](#table:gmset)), explicit memory copies (Table [9.9](#table:gxcopy)), and synchronization (Table [9.10](#table:gsync)). These operation metrics are available for GPUs for AMD, Intel, and NVIDIA GPUs. While summary metrics in Table [9.1](#table:gtimes) and Table [9.2](#table:gker) are shown by default in hpcviewer, metrics marked with an asterisk in Tables [9.7](#table:gmem), [9.8](#table:gmset), [9.9](#table:gxcopy), and [9.10](#table:gsync) are hidden by default to avoid overwhelming users with many columns of metrics. In `hpcviewer`, one can reveal any hidden metric by opening its metric pane marking the metric as visible.

```{table} Table 9.5: GPU operation timings.
---
name: table:gtimes
---
| Metric       | Description                                        |
| :----------- | :------------------------------------------------- |
| GKER (s)   | GPU time: kernel execution (seconds)               |
| GMEM (s)   | GPU time: memory allocation/deallocation (seconds) |
| GMSET (s)  | GPU time: memory set (seconds)                     |
| GXCOPY (s) | GPU time: explicit data copy (seconds)             |
| GSYNC (s)  | GPU time: synchronization (seconds)                |
| GPUOP (s)  | Total GPU operation time: sum of all metrics above |
```

```{table} Table 9.6: GPU kernel characteristic metrics.
---
name: table:gker
---
| Metric          | Description                                 |
| :-------------- | :------------------------------------------ |
| GKER:STMEM (B)  | GPU kernel: static memory (bytes)           |
| GKER:DYMEM (B)  | GPU kernel: dynamic memory (bytes)          |
| GKER:LMEM (B)   | GPU kernel: local memory (bytes)            |
| GKER:FGP_ACT    | GPU kernel: fine-grain parallelism, actual  |
| GKER:FGP_MAX    | GPU kernel: fine-grain parallelism, maximum |
| GKER:THR_REG    | GPU kernel: thread register count           |
| GKER:BLK_THR    | GPU kernel: thread count                    |
| GKER:BLK        | GPU kernel: block count                     |
| GKER:BLK_SM (B) | GPU kernel: block local memory (bytes)      |
| GKER:COUNT      | GPU kernel: launch count                    |
| GKER:OCC_THR    | GPU kernel: theoretical occupancy           |
```

```{table} Table 9.7: GPU memory allocation and deallocation. Note: metrics marked with * are hidden by default.
---
name: table:gmem
---
| Metric       | Description                                          |
| :----------- | :--------------------------------------------------- |
| GMEM:UNK (B)* | GPU memory alloc/free: unknown memory kind (bytes)   |
| GMEM:PAG (B)* | GPU memory alloc/free: pageable memory (bytes)       |
| GMEM:PIN (B)* | GPU memory alloc/free: pinned memory (bytes)         |
| GMEM:DEV (B)* | GPU memory alloc/free: device memory (bytes)         |
| GMEM:ARY (B)* | GPU memory alloc/free: array memory (bytes)          |
| GMEM:MAN (B)* | GPU memory alloc/free: managed memory (bytes)        |
| GMEM:DST (B)* | GPU memory alloc/free: device static memory (bytes)  |
| GMEM:MST (B)* | GPU memory alloc/free: managed static memory (bytes) |
| GMEM:COUNT   | GPU memory alloc/free: count                         |
```

```{table} Table 9.8: GPU memory set metrics. Note: metrics marked with * are hidden by default.
---
name: table:gmset
---
| Metric        | Description                                    |
| :------------ | :--------------------------------------------- |
| GMSET:UNK (B)* | GPU memory set: unknown memory kind (bytes)   |
| GMSET:PAG (B)* | GPU memory set: pageable memory (bytes)       |
| GMSET:PIN (B)* | GPU memory set: pinned memory (bytes)         |
| GMSET:DEV (B)* | GPU memory set: device memory (bytes)         |
| GMSET:ARY (B)* | GPU memory set: array memory (bytes)          |
| GMSET:MAN (B)* | GPU memory set: managed memory (bytes)        |
| GMSET:DST (B)* | GPU memory set: device static memory (bytes)  |
| GMSET:MST (B)* | GPU memory set: managed static memory (bytes) |
| GMSET:COUNT    | GPU memory set: count                         |
```

```{table} Table 9.9: GPU explicit memory copy metrics. Note: metrics marked with * are hidden by default.
---
name: table:gxcopy
---
| Metric         | Description                                         |
| :------------- | :-------------------------------------------------- |
| GXCOPY:UNK (B)* | GPU explicit memory copy: unknown kind (bytes)     |
| GXCOPY:H2D (B)* | GPU explicit memory copy: host to device (bytes)   |
| GXCOPY:D2H (B)* | GPU explicit memory copy: device to host (bytes)   |
| GXCOPY:H2A (B)* | GPU explicit memory copy: host to array (bytes)    |
| GXCOPY:A2H (B)* | GPU explicit memory copy: array to host (bytes)    |
| GXCOPY:A2A (B)* | GPU explicit memory copy: array to array (bytes)   |
| GXCOPY:A2D (B)* | GPU explicit memory copy: array to device (bytes)  |
| GXCOPY:D2A (B)* | GPU explicit memory copy: device to array (bytes)  |
| GXCOPY:D2D (B)* | GPU explicit memory copy: device to device (bytes) |
| GXCOPY:H2H (B)* | GPU explicit memory copy: host to host (bytes)     |
| GXCOPY:P2P (B)* | GPU explicit memory copy: peer to peer (bytes)     |
| GXCOPY:COUNT   | GPU explicit memory copy: count                     |
```

```{table} Table 9.10: GPU synchronization metrics. Note: metrics marked with * are hidden by default.
---
name: table:gsync
---
| Metric            | Description                             |
| :---------------- | :-------------------------------------- |
| GSYNC:UNK (s)*  | GPU synchronizations: unknown kind      |
| GSYNC:EVT (s)*  | GPU synchronizations: event             |
| GSYNC:STRE (s)* | GPU synchronizations: stream event wait |
| GSYNC:STR (s)*  | GPU synchronizations: stream            |
| GSYNC:CTX (s)*  | GPU synchronizations: context           |
| GSYNC:COUNT       | GPU synchronizations: count             |
```

```{important}
The cost of profiling and tracing GPU operations using monitoring libraries provided by GPU vendors can't be controlled by HPCToolkit.
Unlike CPU monitoring based on asynchronous sampling, GPU performance monitoring uses callback interfaces provided by vendor monitoring libraries to monitor the initiation and completion of each GPU operation. Accordingly, the overhead of GPU performance monitoring depends upon how frequently GPU operations are initiated.

For example, when monitoring CUDA applications on NVIDIA GPUs with NVIDIA's CUPTI interface, we have seen the execution time double when profiling and tracing a GPU-accelerated application that launches kernels very frequently. Measures of GPU activity are accurate; however, processing the torrent of measurement data from one or more GPUs can add considerable CPU overhead. For that reason, you should expect some CPU overhead associated with profiling and tracing of GPU operations and account for it in your analysis.

For instance, if a GPU-accelerated program runs in 1000 seconds without HPCToolkit monitoring GPU activity but slows to 2000 seconds when GPU profiling and tracing is enabled, then if GPU profiles and traces show that the GPU is active for 25% of the execution time, one should mentally re-scale the accurate measurements of GPU activity by considering the `2x` dilation that occurs when monitoring GPU activity. Without monitoring, one would expect the same level of GPU activity, but the host time would be twice as fast. Thus, without monitoring, the ratio of GPU activity to host activity would be roughly double.
```

(sec:gpu-tracing)=

## Tracing GPU Operations

HPCToolkit also supports tracing of activities on GPU streams on AMD, Intel, and NVIDIA GPUs when using ROCm, Level Zero, CUDA, OpenCL or OpenMP offloading. Tracing of GPU activities will be enabled any time GPU monitoring is enabled and `hpcrun`'s tracing is enabled with `-t|--trace`.

For tracing GPU-accelerated workloads, HPCToolkit offers a better tracing option `-tt|--ttrace`, which collects a boosted resolution trace of CPU threads.
Like `-t`, `-tt` records a call path trace for CPU threads based on asynchronous sampling of a time-based sampling metric,
such as `CPUTIME`, `REALTIME`, or `cycles`.
Unlike `-t`, `-tt` also records a sample for a CPU thread as it launches each GPU operation (if any).
Without `-tt`, the activity seen on a CPU trace line at the time a kernel is launched may be from long ago, which makes it hard to understand how CPU activity relates to GPU activity.
Note that using `-tt` disturbs the statistical properties of CPU traces since it adds non-sample events whenever a thread launches a GPU operation.

```{important}
The next section describes instruction-level measurement within GPU kernels using PC sampling. Unless you are using a very large sampling period, collecting and attributing PC samples may add significant CPU overhead to a GPU-accelerated code. Tracing is not recommended when PC sampling is enabled as the overhead of PC sampling will likely affect timings in traces.
```

```{important}
During execution `hpcrun` creates CPU tool threads to record traces of GPU activities. The work performed by such tool threads is not reported in profiles and traces shown by HPCToolkit. By default, `hpcrun` creates one tracing thread per 256 GPU streams. To adjust the number of GPU streams per tracing thread, see the settings for `HPCRUN_CONTROL_KNOBS` in Appendix [14](#sec:env).
When mapping a GPU-accelerated node program onto a node, you may need to consider provisioning additional hardware threads or cores to accommodate these tracing threads; otherwise, they may compete with application threads for CPU resources, which may degrade the performance of your program.
```

(sec:gpu-sampling)=

## PC Sampling within GPU Kernels

HPCToolkit collects Program Counter (PC) samples to measure instruction-level performance metrics on AMD, Intel and NVIDIA GPUs. Measurement of PC samples is supported for CUDA on NVIDIA GPUs, HIP on AMD GPUs, SYCL/DPC++ on Intel GPUs, and OpenMP on all GPUs. None of the GPU vendors support PC sampling for OpenCL.

All GPU implementations of PC sampling periodically interrupt execution of a kernel warp or wavefront running on a GPU to record the value of its program counter to identify the instruction that an AMD compute unit, an Intel execution unit, or an NVIDIA streaming multiprocessor is trying to execute. HPCToolkit attributes much of the information gathered from sample records in a vendor-neutral way.
HPCToolkit also supports instruction-level measurement on Intel GPUs using binary instrumentation.

PC sampling on Intel and NVIDIA GPUs is supported by hardware. AMD GPUs have two strategies for collecting PC samples: one supported entirely in software and one supported by hardware. Table [9.11](#table:gpu:cycles) shows three high-level metrics that can be collected using PC sampling: estimates of cycles (`GCYCLES`), instruction issues (`GCYCLES:ISU`), and *exposed* stalls (`GCYCLES:STL`). These metrics are estimates for two reasons.

```{table} Table 9.11: GPU cycles are issues or exposed stalls.
---
name: table:gpu:cycles
---
| Metric          | Description                                                          |
| :-------------- | :------------------------------------------------------------------- |
| GCYCLES         | GPU cycles (estimated using PC sampling)                             |
| GCYCLES:ISU     | GPU issue cycles: a sampled instruction was issued by the front end  |
| GCYCLES:STL     | GPU stall cycles: a sampled instruction represents an exposed stall  |
```

First, when sampling with a period of `N` instructions, each time a sample is recorded, HPCToolkit charges `N` cycles to the machine instruction indicated by the program counter. This strategy has the advantage that the measurement results should be independent of the sampling period. NVIDIA GPUs and AMD's hardware-supported stochasitic sampling both use sampling periods specified using a number of instructions. When measuring a program using PC sampling, one can specify a sampling period by adding an `@k` to the end of the argument enabling PC sampling; this will set the sampling period to `2^k`.

Second, Intel's Level Zero runtime and AMD's software implementation of PC sampling for MI200+ GPUs specify the sampling period as time rather than the number of instructions. For consistency, we use nanoseconds as the unit of time. Like the aforementioned implementations with instruction-level periods, HPCToolkit's implementation of PC sampling for Intel GPUs and AMD's software implementation of PC sampling use `@k` to indicate a sampling period of 2^k; however, the period for these implementations is in nanoseconds rather than cycles. For both Intel and AMD GPUs, base clock frequencies are approximately 1GHz. While boosted clock frequencies can be as much as twice as fast, HPCToolkit doesn't attempt to correct for that. Instead, HPCToolkit treats nanoseconds and clock cycles as interchangeable. Even when using time-based periods, HPCToolkit reports GPU measurements as `GCYCLES`. With clock scaling, this may be off by as much as a factor of two. However, for the purpose of identifying which source lines within kernels consume resources, metrics will accumulate at the proper locations with the proper relative costs for different locations regardless of whether the metric actually is a measure of GPU cycles or nanoseconds.

When using AMD's software support for PC sampling, `GCYCLES` is the only metric that can be collected; when processing a PC sample at runtime, no information is available about whether the sampled instruction issued or stalled. In contrast, hardware support for PC sampling on AMD, Intel, and NVIDIA GPUs reports whether the sampled instruction was issued (`GCYCLES:ISU`) or whether the sampled instruction stalled (`GCYCLES:STL`) and was not issued. As with `GCYCLES`, measurements of `GCYCLES:ISU` and `GCYCLES:STL` are scaled by multiplying the actual count of samples by the sample period.

HPCToolkit considers a PC sample to represent a stall (`GCYCLES:STL`) only if the latency of the instruction's stall is exposed. On an NVIDIA GPU, HPCToolkit only considers a sampled instruction to be a stall if NVIDIA's CUPTI reports the instruction as a latency sample, which means that no instruction issued on the GPU in the cycle when the sample was recorded. On an AMD GPU, AMD's Rocprofiler-sdk reports the type of instruction that issued. AMD instruction types are shown in Table [9.14](#table:amd-issues). HPCToolkit only considers an instruction of type `X` to be an exposed stall if instruction `X` should issue to pipeline `Y`, and pipeline `Y` did not issue an instruction in the current cycle.

Each of the GPUs report when a sampled instruction is ready to execute but was `NOT_SELECTED`. HPCToolkit doesn't consider `NOT_SELECTED` as an exposed stall. An instruction that is `NOT_SELECTED` stalled only because another instruction is executing instead. When the latency of a stalled instruction is overlapped with execution of another instruction, a developer need not be concerned about the reason for the stall. GPUs are designed to hide the latency of stalled instructions by interleaving the execution of instructions from different warps or wavefronts and overlapping one warp or wavefront's stall with execution of an instruction from another warp or wavefront.

Besides reporting the summary metric `GCYCLES:STL` of exposed stalls, HPCToolkit additionally measures and attributes reasons for exposed stalls. HPCToolkit maps stall reasons from AMD, Intel, and NVIDIA GPUs into a (mostly) vendor-neutral taxonomy of stall reasons. Table [9.12](#table:issue-stall) shows stall metrics recorded by HPCToolkit using hardware support for PC sampling on AMD, Intel, and NVIDIA GPUs.

```{table} Table 9.12: GPU issue stall metrics. Note: metrics marked with * are hidden by default.
---
name: table:issue-stall
---
| Metric            | Description                                                                                                 |
| :---------------- | :---------------------------------------------------------------------------------------------------------- |
| GCYCLES:STL       | GPU exposed instruction issue stall cycles: any kind                                                        |
| GCYCLES:STL:MEM*   | GPU exposed instruction issue stall cycles: await completion of a kind of memory access                     |
| GCYCLES:STL:GMEM*  | GPU exposed instruction issue stall cycles: await completion of a global memory access                      |
| GCYCLES:STL:MTHR* | GPU exposed instruction issue stall cycles: global memory request queue full                                |
| GCYCLES:STL:TMEM* | GPU exposed instruction issue stall cycles: texture memory request queue full                               |
| GCYCLES:STL:CMEM* | GPU exposed instruction issue stall cycles: await completion of constant or immediate memory access         |
| GCYCLES:STL:IFET* | GPU exposed instruction issue stall cycles: await availability of next instruction (fetch or branch delay   |
| GCYCLES:STL:IDEP* | GPU exposed instruction issue stall cycles: await satisfaction of instruction input dependence              |
| GCYCLES:STL:PIPE* | GPU exposed instruction issue stall cycles: await completion of required compute resources                  |
| GCYCLES:STL:SYNC* | GPU exposed instruction issue stall cycles: await completion of thread or memory synchronization            |
| GCYCLES:STL:OTHR* | GPU exposed instruction issue stall cycles: other                                                           |
| GCYCLES:STL:SLP*  | GPU exposed instruction issue stall cycles: sleep                                                           |
```

In some cases, the mapping is precise. For instance, NVIDIA GPUs report separate stall reasons for stalls related to global memory (`GCYCLES:STL:GMEM`), texture memory (`GCYCLES:STL:TMEM`), and constant memory (`GCYCLES:STL:CMEM`). In contrast, AMD GPUs report stalls that arise from `waitcnt` instructions, which await completion of loads that may access the Local Data Store (LDS) or device memory. Accordingly, HPCToolkit reports all memory-related stalls on AMD GPUs using a single metric `GCYCLES:STL:MEM`. On Intel GPUs, reporting is a bit more muddled. Memory-related stalls are typically reported as SCOREBOARD ID stalls or SEND stalls. We report Intel SCOREBOARD ID stalls as `GCYCLES:STL:MEM` and SEND stalls as `GCYCLES:STL:GMEM` since SEND instructions are used to send messages to other components or write to device memory.

### Attributing PC Samples to Source Code

Using PC sampling, HPCToolkit attributes GPU metrics to heterogeneous calling contexts that include the CPU calling context in which a GPU kernel was launched as a prefix and measurements within the GPU kernel as the suffix.

When collecting PC samples for a GPU-accelerated applications, GPU runtimes notify `hpcrun` every time they load a binary into a GPU.
The first time a GPU binary is loaded, `hpcrun` computes a cryptographic hash of the GPU binary's contents and records the binary inside a `gpubins` subdirectory of `hpcrun`'s measurement directory in a file with a name based on the hex representation of its cryptographic hash, e.g., `984ef7dea.gpubin`.

To attribute PC samples collected during an execution of a GPU-accelerated application back to the application's code, one simply follows the traditional HPCToolkit workflow. Invoking `hpcstruct` on the execution's measurement directory analyzes any GPU binaries used in the course of an execution along with the executable and CPU shared libraries. Program structure information recovered by `hpcstruct` is used to map PC samples associated with machine instructions back to source code.

By default, when applied to a measurements directory, `hpcstruct` performs only lightweight analysis of the GPU functions in each GPU binary, enabling PC samples to be mapped to functions, loops, and source lines. It is important to understand that AMD, Intel, and NVIDIA GPU measurement APIs for PC sampling collect "flat" PC samples without any information about GPU call stacks.

In our experience, calling contexts within GPU kernels are essential for developers to understand the performance of sophisticated kernels that employ many device functions. The GPU-accelerated [Quicksilver](https://asc.llnl.gov/codes/proxy-apps/quicksilver) proxy application from Lawrence Livermore National Laboratory illustrates this problem. Figure [9.1](#qs-no-cct) shows a screenshot of `hpcviewer` displaying PC sampling measurements for Quicksilver without reconstruction of a calling context tree within `CycleTrackingKernel`. The figure shows a top-down view of heterogeneous calling contexts that span both CPU and GPU. In the middle of the figure is a placeholder `<gpu kernel>` that is inserted by HPCToolkit to mark the transition between CPU and GPU code in the call stack. Above the placeholder is a CPU calling context where the GPU kernel was invoked. Below the `<gpu kernel>` placeholder, `hpcviewer` shows more than a dozen GPU device functions that were executed on behalf of `CycleTrackingKernel`.

```{figure-md} qs-no-cct
![](qs-no-cct.png)

Figure 9.1: A screenshot of `hpcviewer` for the GPU-accelerated Quicksilver proxy app without GPU CCT reconstruction.
```

Since GPU measurement APIs don't provide information about device-function call stacks within GPU kernels for PC samples, we designed a method to reconstruct GPU calling contexts during post-mortem analysis. This analysis is only performed when (1) an execution has been monitored using PC sampling, and (2) the measurement directory containing an execution's GPU binaries has been analyzed in detail by invoking `hpcstruct` on the directory with the option `--gpucfg yes`. Reconstructing calling contexts within GPU kernels is optional because it may be expensive for kernels with large calling context trees.

To reconstruct calling context trees for GPU computations, HPCToolkit uses information about call sites identified by `hpcstruct` in conjunction with PC samples measured for each `call` instruction in a GPU binary.

Apportioning costs among two or more calls to the same device function in a GPU calling context is a heuristic process.
Without the ability to measure each device function invocation, HPCToolkit assumes that each invocation of a particular device function incurs the same costs. The costs of each device function are apportioned among its caller or callers using the following rules:

- If a device function G can only be invoked from a single call site, all of the measured cost of G will be attributed to its single call site.

- If a device function G can be called from multiple call sites and PC samples have been collected for one or more of the call instructions for G, the costs for G are proportionally divided among G's call sites according to the distribution of PC samples for calls that invoke G. For instance, consider the case where there are three call sites where G may be invoked, 5 samples are recorded for the first call instruction, 10 samples are recorded for the second call instruction, and no samples are recorded for the third. In this case, HPCToolkit divides the costs for G among the first two call sites, attributing 5/15 of G's costs to the first call site and 10/15 of G's costs to the second call site.

- If no call instructions for a device function G have been sampled, the costs of G are apportioned evenly among each of G's call sites.

HPCToolkit's `hpcprof` analyzes the static call graph associated with each GPU kernel. If the static call graph for a GPU kernel contains cycles, which arise from recursive or mutually-recursive calls, `hpcprof` replaces each cycle with a [strongly connected component](https://en.wikipedia.org/wiki/Strongly_connected_component) (SCC). In this case, `hpcprof` unlinks call graph edges between vertices within the SCC and adds an SCC vertex to enclose the set of vertices in each SCC. The rest of `hpcprof`'s analysis treats an SCC vertex in the call graph like a device function. The process of reconstructing calling context trees for GPU kernels using static call sites and sample counts is described [elsewhere](https://arxiv.org/pdf/2109.06931).

```{figure-md} qs-cct
![](qs-cct.png)

Figure 9.2: A screenshot of `hpcviewer` for the GPU-accelerated Quicksilver proxy app with GPU CCT reconstruction.
```

Figure [9.2](#qs-cct) shows an `hpcviewer` screenshot for the GPU-accelerated Quicksilver code following reconstruction of GPU calling contexts. Notice that after the reconstruction, one can see that `CycleTrackingKernel` calls `CycleTrackingGuts`, which calls `CollisionEvent`, which eventually calls `macroscopicCrossSection` and `NuclearData::getNumberOfReactions`. The the rich GPU calling context tree reconstructed by `hpcprof` also shows loop nests and inlined code.

```{tip}
If all PC samples in a kernel map to line 0, it is likely that the GPU binaries used by your application don't contain line mapping information. Consult the appropriate GPU-specific section below for information about the compiler option required to record line mappings that you need for performance analysis.
```

(sec:gpu-cuda)=

## NVIDIA GPUs

HPCToolkit supports profiling, tracing, and PC sampling of GPU operations on NVIDIA GPUs for programs written in CUDA, OpenMP, OpenCL. Profiling, tracing, and PC sampling of GPU-accelerated applications have already been in this chapter. In this section, we focus on a few things specific to the measurement of applications on NVIDIA GPUs.

(sec:nvidia-pc-sampling)=

### PC Sampling on NVIDIA GPUs

NVIDIA's GPUs have supported [PC sampling](https://docs.nvidia.com/cupti/Cupti/r_main.html#r_pc_sampling) since Maxwell.
Instruction samples are collected separately on each active streaming
multiprocessor (SM) and merged in a buffer returned by NVIDIA's CUPTI.
In each sampling period, one warp scheduler of each active SM
samples the next instruction from one of its active warps. Sampling rotates through
an SM's warp schedulers in a round robin fashion.
When an instruction is sampled, its stall reason (if any) is
recorded. If all warps on a scheduler are stalled when a sample is
taken, the sample is marked as a latency sample, meaning no instruction will be issued by the warp scheduler in the next cycle.
Figure [9.3](#fig:pc-sampling) shows a PC sampling example on an SM with four schedulers. Among the six collected samples, four are latency samples, so the estimated stall ratio is 4/6.

Table [9.13](#table:gsamp) shows summary statistics recorded by HPCToolkit with collecting PC samples on NVIDIA GPUs. Of particular note is the metric `GSAMP:UTIL`. HPCToolkit computes approximate GPU utilization using information gathered using PC sampling. Given the average clock frequency and the sampling rate, if all SMs are active, then HPCToolkit knows how many instruction samples would be expected (`GSAMP:EXP`) if the GPU was fully active for the interval when it was in use. HPCToolkit approximates the percentage of GPU utilization by comparing the measured samples with the expected samples using the following formula: `100 * (GSAMP:TOT) / (GSAMP:EXP)`.

```{figure-md} fig:pc-sampling
![](mental-model.png)

Figure 9.3: NVIDIA's GPU PC sampling example on an SM. `P`-`6P` represent
six sample periods P cycles apart. `S_1`-`S_4` represent four schedulers on an SM.
```

```{table} Table 9.13: GPU utilization statistics computed from PC Samples on NVIDIA GPUs.
---
name: table:gsamp
---
| Metric          | Description                                |
| :-------------- | :----------------------------------------- |
| GSAMP:DRP       | GPU PC samples: dropped                    |
| GSAMP:EXP       | GPU PC samples: expected                   |
| GSAMP:TOT       | GPU PC samples: measured                   |
| GSAMP:PER (cyc) | GPU PC samples: period (GPU cycles)        |
| GSAMP:UTIL (%)  | GPU utilization computed using PC sampling |
```

```{important}
To collect PC samples on NVIDIA GPUs, at present HPCToolkit uses an older CUPTI interface that serializes the execution of GPU kernels. HPCToolkit uses this interface so that it can precisely attribute GPU PC samples to CPU calling contexts that invoke GPU kernels. This interface blocks concurrent execution of GPU kernels, which may slow execution. Furthermore, because of kernel serialization, HPCToolkit's PC sampling measurements will only reflect the GPU activity of kernels running in isolation.

Our experience with CUPTI's serialization-based API for PC sampling is that the overhead is less than `5x`. The overhead of GPU monitoring is principally on the host side. The time spent in GPU operations or in kernels as measured by PC samples is expected to be accurate. However, since execution as a whole is slowed while measuring GPU operations and collecting PC samples, one must be careful about drawing conclusions about overall program performance. Any traces collected while collecting PC samples on NVIDIA GPUs will be dilated and should be viewed as providing qualitative information only.
```

### Attributing PC Samples to CUDA Source Code

To attribute performance within GPU kernels, one must add an appropriate flag to NVIDIA's compilers to record mappings between machine code and source code. `nvcc` and `nvc++` provide a `-lineinfo` option that records precise mappings from machine code to source code, including information about inlined functions.

```{important}
Don't use `nvcc`'s `-G` option when measuring performance. While it records precise mappings from machine code to source code, it also disables *all* compiler optimizations and generates GPU code that may be vastly slower.
```

### Binary analysis on NVIDIA GPUs

Analysis of binaries for NVIDIA GPUs is problematic. NVIDIA does not provide an API for analysis of GPU binaries. For that reason, HPCToolkit invokes NVIDIA's `nvdisasm` command line tool to analyze control flow in NVIDIA GPU binaries. `nvdisasm` produces Control Flow Graphs (CFGs) in DOT with assembly code annotations for each node in a CFG. For large CUDA binaries, `nvdisasm`'s ASCII output files can be huge; however, that is the least of the problems. The biggest problems are that (1) `nvdisasm` fails to produce a CFG for GPU functions that have certain features, e.g. contain `sib calls`, and (2) when a GPU binary contains a function that `nvdisasm` can't process, it gives up and doesn't process the remainder of a GPU binary.

To compensate, HPCToolkit inspects the symbol table of an NVIDIA GPU binary and then invokes `nvdisasm` once for each function in the GPU binary to extract that function's CFG. With this approach, HPCToolkit is resilient against `nvdisasm` deficiencies. On any function where `nvdisasm` fails to extract a CFG, HPCToolkit attributes costs any samples collected to the function as a whole.

For large binaries with many functions, the cost of invoking `nvdisasm` to extract a CFG for just one function is not proportional to only the size of the function of interest. That may make the aforementioned approach impractical for very large binaries. When this approach was applied to an NVIDIA GPU binary almost a gigabyte in size containing roughly 40K functions, `hpcstruct` analyzed the binary for two days without completing.

If `hpcstruct` takes too long to analyze anything, it can be interrupted with `^C`. Any analysis already complete, will be saved and not recomputed. Restarting `hpcstruct` will show the first binary where analysis is being reinitiated. A problematic binary can be excluded from analysis by `hpcstruct` using `hpcstruct`'s `-x|--exclude` option. Without program structure information from `hpcstruct`, HPCToolkit's `hpcprof` will be unable to attribute costs to calling contexts within a GPU kernel, loops, or inlined code. However, `hpcprof` will still be able to attribute PC samples to source lines in kernels and device functions.

(sec:gpu-rocm)=

## AMD GPUs

On AMD GPUs,
HPCToolkit supports coarse-grain profiling of GPU-accelerated applications that offload GPU computation using AMD's HIP programming model, OpenMP, and OpenCL. In this section, we focus on a few important details about monitoring applications on AMD GPUs with AMD's new [Rocprofiler-sdk](https://rocm.docs.amd.com/projects/rocprofiler-sdk).

### PC Sampling on AMD GPUs

AMD GPUs support two kinds of PC sampling: host-trap (software-based) sampling and stochastic (hardware-based) sampling. Support for host-trap based PC sampling first became available with ROCm 6.4 and is available on AMD GPUs MI200 and newer.
Support for stochastic sampling first became available with ROCm 7.0 and it requires hardware support that first became available with AMD's MI300 series. Both kinds of sampling need the GPU to be running modern firmware.

You can check whether one or both kinds of PC sampling are available for your GPUs with AMD's [`rocprofv3-avail`](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/latest/how-to/using-rocprofv3-avail.html) utility. Running `rocprofv3-avail info --pc-sampling` will report the list of PC sampling configurations supported for each GPU agent in your system.

```{tip}
If neither form of PC sampling is available, your GPU is an MI200+, and ROCm 6.4+ is installed, you might want to discuss with your system administrator whether PC sampling can be made available with a firmware update. Some information about what firmware versions are necessary or recommended for PC sampling on various GPU versions can be found in the [Rocprofiler-sdk documentation](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/latest/how-to/using-pc-sampling.html). If you don't find what you need for your GPU, check with AMD by submitting a Github issue for [AMD's documentation](https://github.com/ROCm/rocm-systems/tree/develop/projects/rocprofiler-sdk/source/docs).
```

PC sampling on AMD GPUs is a device-wide activity for both host-trap (software-based) sampling and stochastic (hardware-based) sampling.

When using host-trap (software-based) PC sampling, a background thread periodically interrupts running waves on a GPU to read the program counter.
Samples collected using host-trap sampling may suffer from "skid", where a sample may be attributed to an instruction up to two instructions away from a source of latency. As described in AMD's [documentation](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/latest/how-to/using-pc-sampling.html#host-trap-pc-sampling-and-arbitrary-sampling-skid) about host-trap sampling, one must be careful interpreting host-trap PC samples because the costly instructions may be nearby rather than the ones where costs are attributed.

When using stochastic (hardware-based) PC sampling, each compute unit periodically chooses an active wave on the compute unit and records its PC. Over time, each compute unit samples waves in a round robin fashion.

To select software-based host-trap PC sampling, specify `-e gpu=rocm,pc=sw`.
To select hardware-based stochastic PC sampling, specify `-e gpu=rocm,pc=hw`.
Specifying simply `-e gpu=rocm,pc` will default to software-based host-trap sampling.

Periods for hardware stochastic sampling are measured in GPU cycles and the minimum is 256; periods must be a power of 2. The default period for stochastic sampling is currently set to `2^20` cycles. HPCToolkit enables a user to specify the desired period by adding an `@k` to the end of a PC sampling specification, e.g. `-e gpu=rocm,pc=hw@22` will set the period for stochastic sampling to `2^22` cycles.
Periods for host-trap based sampling are measured in microseconds. The minimum period is 1 us, which HPCToolkit enforces as `2^10` ns. The default period for host-trap sampling is set to `2^17` ns.
Using `-e gpu=rocm,pc=sw@18` will set the period for software-based host-trap sampling to `2^18` ns.

```{important}
At present, setting the sampling period for `hw` PC sampling to a period significantly shorter than `2^20` has been observed to cause AMD's driver to fail. As a result, HPCToolkit uses the AMD-recommended default PC sampling period of `2^20` cycles for stochastic sampling.
```

For both kinds of sampling, HPCToolkit reports `GCYCLES` or GPU cycles. The default frequency for AMD GPUs is approximately 1GHz, so 1 cycle is approximately 1 ns. While frequency adjustments may affect the GPU clock, assuming that the frequency is constant at 1GHz should be sufficient for measurements to be adequate for performance tuning.

AMD's stochastic sampling hardware records a wealth of information with each PC sample. A sampled instruction will either issue or not. In most cases, the stochastic sampling hardware in an AMD MI300 reports the kind of a wavefront's sampled instruction. Table [9.11](#table:amd-issues) shows the kinds of issued instructions that the MI300 tracks and reports. In some cases, a wavefront's instruction may not be known if the wavefront is stalled fetching the next instruction after a branch.

```{table} Table 9.14: GPU issue metrics. Note: metrics marked with * are hidden by default.
---
name: table:amd-issues
---
| Metric            | Description                                                               |
| :---------------- | :------------------------------------------------------------------------ |
| GCYCLES:ISU:MATR* | GPU issue cycles: issued a sampled matrix instruction                     |
| GCYCLES:ISU:VEC2* | GPU issue cycles: issued dual vector instructions                         |
| GCYCLES:ISU:VEC*  | GPU issue cycles: issued a sampled vector instruction                     |
| GCYCLES:ISU:SCLR* | GPU issue cycles: issued a sampled scalar instruction                     |
| GCYCLES:ISU:TEX*  | GPU issue cycles: issued a sampled texture instruction                    |
| GCYCLES:ISU:LDS*  | GPU issue cycles: issued a sampled Local Data Store instruction           |
| GCYCLES:ISU:LDSD* | GPU issue cycles: issued a sampled Local Data Store direct instruction    |
| GCYCLES:ISU:FLAT* | GPU issue cycles: issued a sampled flat instruction                       |
| GCYCLES:ISU:XPRT* | GPU issue cycles: issued a sampled export instruction                     |
| GCYCLES:ISU:MESG* | GPU issue cycles: issued a sampled message instruction                    |
| GCYCLES:ISU:BAR*  | GPU issue cycles: issued a sampled barrier instruction                    |
| GCYCLES:ISU:BRT*  | GPU issue cycles: issued a sampled taken branch instruction               |
| GCYCLES:ISU:BRNT* | GPU issue cycles: issued a sampled not taken branch instruction           |
| GCYCLES:ISU:JMP*  | GPU issue cycles: issued a sampled jump instruction                       |
| GCYCLES:ISU:OTHR* | GPU issue cycles: issued a sampled 'other' instruction                    |
```

AMD GPUs support multiple execution pipelines and multiple instruction issue. As a result, more than one pipeline may report an issue in a cycle. AMD's stochastic sampling hardware tracks all instructions that issue in a sampled cycle, not just the sampled instruction. Different models of AMD GPUs may have different pipelines. HPCToolkit reports AMD GPU pipeline issues with a separate metric for each GPU pipeline, as shown in Table [9.15](#table:pipe-issue). Note that matrix pipeline is only used for matrix instructions on low-precision numbers. Matrix operations on 32 or 64-bit numbers issue to the vector pipeline.

```{table} Table 9.15: GPU pipeline issue metrics.
---
name: table:pipe-issue
---
| Metric         | Description                                                     |
| :------------- | :-------------------------------------------------------------- |
| GPIPE:ISU:MATR | GPU pipeline issue status: low precision matrix operation       |
| GPIPE:ISU:VEC2 | GPU pipeline issue status: vector, dual                         |
| GPIPE:ISU:VEC  | GPU pipeline issue status: vector                               |
| GPIPE:ISU:SCLR | GPU pipeline issue status: scalar ALU or memory                 |
| GPIPE:ISU:LDS  | GPU pipeline issue status: Local Data Store                     |
| GPIPE:ISU:LDSD | GPU pipeline issue status: Local Data Store direct              |
| GPIPE:ISU:TEX  | GPU pipeline issue status: texture                              |
| GPIPE:ISU:FLAT | GPU pipeline issue status: flat                                 |
| GPIPE:ISU:XPRT | GPU pipeline issue status: export                               |
| GPIPE:ISU:BMSG | GPU pipeline issue status: branch or message                    |
| GPIPE:ISU:MISC | GPU pipeline issue status: miscellaneous                        |
```

A GPU pipeline may stall waiting for resources that are not available in the current cycle, e.g., register ports.
HPCToolkit reports AMD GPU pipeline stalls with a separate metric for each GPU pipeline, as shown in Table [9.16](#table:pipe-stall). By comparing issue and stall counts with the GCYCLES metric, one can compute the fraction of cycles in which each pipeline issues, stalls, or is idle.

```{table} Table 9.16: GPU pipeline stall metrics.
---
name: table:pipe-stall
---
| Metric          | Description                                                     |
| :-------------- | :-------------------------------------------------------------- |
| GPIPE:STL:MATR  |  GPU pipeline stall status: low precision matrix operation      |
| GPIPE:STL:VEC2  | GPU pipeline stall status: vector, dual                         |
| GPIPE:STL:VEC   | GPU pipeline stall status: vector                               |
| GPIPE:STL:SCLR  | GPU pipeline stall status: scalar ALU or memory                 |
| GPIPE:STL:LDS   | GPU pipeline stall status: Local Data Store                     |
| GPIPE:STL:LDSD  | GPU pipeline stall status: Local Data Store direct              |
| GPIPE:STL:TEX   | GPU pipeline stall status: texture                              |
| GPIPE:STL:FLAT  | GPU pipeline stall status: flat                                 |
| GPIPE:STL:XPRT  | GPU pipeline stall status: export                               |
| GPIPE:STL:BMSG  | GPU pipeline stall status: branch or message                    |
| GPIPE:STL:MISC  | GPU pipeline stall status: miscellaneous                        |
```

Computation on AMD's GPUs is organized as wavefronts. AMD GPUs hide latency by overlapping stalls of one wavefront with the execution of another. Having multiple wavefronts available to schedule is important for hiding latency. On AMD GPUs, HPCToolkit measures how many wavefronts are active in each sampled cycle and computes the metrics shown in Table [9.17](#table:wave-util).
`GCYCLES:WAVE_ACT` reports the total number of wavefronts active aggregated across over all sampled cycles. `GCYCLES:WAVE_AVL` reports the total number of wave slots (today 32) aggregated over all sampled cycles. From these two metrics, HPCToolkit computes `GCYCLES:WAVE_UTL` -- the percent utilization of the available wave slots across all sampled cycles. Note that this measure of wave utilization only represents the utilization of wave slots in active CUs. You may have a high wave utilization even if you are only using a small fraction of the available compute units. Wavefront utilization metrics are attributed to code within GPU kernels at all levels of granularity: source lines, loops, inlined functions, and call chains.

```{table} Table 9.17: GPU wave utilization metrics.
---
name: table:wave-util
---
| Metric            | Description                                                                          |
| :---------------- | :----------------------------------------------------------------------------------- |
| GCYCLES:WAVE_ACT  | GPU waves aggregate active                                                           |
| GCYCLES:WAVE_AVL  | GPU waves aggregate available                                                        |
| GCYCLES:WAVE_UTL  | GPU waves utilization (actual occupancy): 100*(waves active)/(waves available)       |
```

Today, AMD's GPUs typically use wavefronts of 64 threads. Using hardware support for stochastic sampling on AMD MI300+ GPUs, HPCToolkit measures how many threads are active and computes the thread activity metrics shown in
Table [9.18](#table:thread-util). When a vector instruction is sampled, HPCToolkit determines how many threads are active in the sampled cycle by counting the number of bits in the execution mask.
`GCYCLES:THR_ACT` reports the total number of bits in the execution mask summed over all sampled vector instructions. `GCYCLES:THR_AVL` reports the wavefront size x the number of sampled vector instructions. From these two metrics, HPCToolkit computes `GCYCLES:THR_UTL` -- the percent utilization of the available SIMD lanes by vector instructions. Thread utilization metrics are attributed to code within GPU kernels at all levels of granularity: source lines, loops, inlined functions, and call chains.

```{table} Table 9.18: GPU thread utilization metrics.
---
name: table:thread-util
---
| Metric            | Description                                                                          |
| :---------------- | :----------------------------------------------------------------------------------- |
| GCYCLES:THR_ACT   | GPU SIMD lanes (threads) aggregate active                                            |
| GCYCLES:THR_AVL   | GPU SIMD lanes (threads) aggregate available                                         |
| GCYCLES:THR_UTL   | GPU SIMD lanes (threads) utilization: 100*(SIMD lanes active)/(SIMD lanes available) |
```

### AMD GPU Hardware Counters

AMD GPUs now support a variety of hardware counters. Using AMD's Rocprofiler-sdk, HPCToolkit can configure a GPU hardware counter to count the number of events that occur during a kernel execution. Multiple counters can be used in the same execution. To see the list of available GPU hardware counters, run the `hpcrun -L` command and scan for counters listed with the prefix `rocm::`. HPCToolkit exposes the set of counters exposted by AMD's Rocprofiler-sdk.

When using HPCToolkit on a system with multiple GPUs, any hardware counters specified on `hpcrun`'s command line will be configured for each of the GPUs specified in `ROCR_VISIBLE_DEVICES` or all GPUs if `ROCR_VISIBLE_DEVICES` is not specified.

If not all of a node's GPUs are not the same kind, they may support different sets of counters. If you encounter a situation where `hpcrun -L` indicates that certain counters are available but you have trouble using them to monitor an execution on multiple GPUs, you can use AMD's utility [`rocprofv3-avail`](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/latest/how-to/using-rocprofv3-avail.html) to check whether a set of counters (e.g., `pmc1`, `pmc2`, `pmc3`) are available for particular GPU device and whether they can be measured together using `rocprofilerv3-avail pmc-check pmc1 pmc2 pmc3`.

Note that the ROCm counter names you use with HPCToolkit have the prefix `rocm::`; when you pass counter names to `rocprofilerv3-avail`, omit the `rocm::` prefix.

(sec:gpu-level0)=

## Intel GPUs

HPCToolkit supports profiling, tracing, and PC sampling of GPU-accelerated applications that offload computation onto Intel GPUs using Intel's Data-parallel C++ programming model supported by Intel's `icpx` compiler, OpenMP computations offloaded with Intel's `ifx`, `icx`, or `icpx` compilers, or OpenCL. At program launch, a user can select whether Intel's Data-parallel C++ programming model is to execute atop Intel's OpenCL runtime or Intel's Level Zero runtime.

Collection of profiles, traces, and PC samples for GPU-accelerated applications was previously described in this chapter. On Intel GPUs, HPCToolkit also supports instrumentation-based measurement of GPU kernels using Intel's GTPin binary instrumentation tool in conjunction with Intel's Level Zero runtime.

At present, HPCToolkit only supports instrumentation-based collection of dynamic instruction counts within GPU kernels. Previously, HPCToolkit also supported approximate measurement of memory latency and a variety of statistics for SIMD instructions. However, problems experienced over the years with various versions of GTPin have led us to disable collection of latency and SIMD measurements in the current release of HPCToolkit. These capabilities may work with the most recent version of GTPin but they are untested.
Instrumentation can be combined with profiling and tracing in the same execution.

### PC Sampling on Intel GPUs

Hardware support for PC sampling on Intel GPUs report stall reasons for sampled instructions. Table [9.19](#intel-gpu-stall-reasons) shows the mapping of stall reasons reported by Intel's GPUs into HPCToolkit's (mostly) vendor-neutral stall reasons. As shown in the table, memory dependency stalls can apparently be reported in two ways. Since memory stalls tend to dominate anything else, we attribute all stalls that contain memory dependencies to memory stalls. On Intel GPUs, we report `SCOREBOARD ID` stalls to `GCYCLES:STL:MEM` to indicate a stall on some unspecified memory, and `SEND` stalls to `GCYCLES:STL:GMEM` (representing global memory stalls) as per our (possibly erroneous) understanding of Intel's stall reasons.

```{table} Table 9.19: Intel GPU stall reasons, their explanations, and the mapping to HPCToolkit's vendor neutral stall reason names.
---
name: intel-gpu-stall-reasons
---
| Intel GPU Stall Reason    | Stall Reason Description                                                | Mapping to HPCToolkit stall reason |
| :------------------------ | :---------------------------------------------------------------------- | :--------------------------------- |
| ACTIVE                    | Actively executing in at least one pipeline                             | Not applicable; not a stall        |
| INST_FETCH                | Stalled due to an instruction fetch operation                           | GCYCLE:STL:IFET                    |
| SYNC                      | Stalled due to sync operation                                           | GCYCLES:STL:SYNC                   |
| SCOREBOARD ID             | Stalled due to memory dependency or internal XVE pipeline dependency    | GCYCLES:STL:MEM                    |
| DIST or ACC               | Stalled due to internal pipeline dependency                            | GCYCLES:STL:OTHR                   |
| PIPESTALL                 | Stalled due to XVE pipeline                                             | GCYCLES:STL:PIPE                   |
| SEND                      | Stalled due to memory dependency or pipeline dependency for send        | GCYCLES:STL:GMEM                   |
| CONTROL                   | Stalled due to branch                                                   | GCYCLE:STL:IFET                    |
| OTHER                     | Stalled due to any other reason                                         | GCYCLE:STL:OTHR                    |
```

### Attributing PC Samples to Source Code

To attribute performance within GPU kernels, one must add appropriate flags to Intel's compilers to record mappings between machine instructions and source code. Intel recommends using compiler options `-gline-tables-only` and `-fdebug-info-for-profiling` rather than `-g`.
