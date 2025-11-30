<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

(chpt:openmp)=

# Monitoring OpenMP

HPCToolkit includes an implementation of the OpenMP Tools API, known as OMPT, initially defined in OpenMP 5.0.
The OMPT interface enables HPCToolkit to extract enough information to reconstruct user-level calling contexts from implementation-level measurements.

No special switches are needed to monitor OpenMP threads, tasks, and offloading with HPCToolkit.
HPCToolkit enables the OMPT interface for monitoring OpenMP by default.

In the unlikely event that there is a bad interaction between HPCToolkit's support for the OMPT interface and an OpenMP runtime, OMPT support may be disabled when measuring your code with HPCToolkit by setting an environment variable, as shown below:

`export OMP_TOOL=disabled`

## Monitoring OpenMP on the Host

Support for OpenMP 5.0 and OMPT is available in OpenMP runtimes for LLVM, AMD, Intel, and IBM compilers. Support in these implementations is mostly complete,
although there are some quirks with OMPT support for tracking computation offloaded on TARGET devices.

A popular OpenMP runtime that lacks OMPT support is the GCC compiler suite's `libgomp`. Fortunately, the LLVM OpenMP runtime, which supports OMPT, is compatible with `libgomp`, at least on the host.

In OpenMP implementations without support for the OMPT interface, HPCToolkit records and reports implementation-level measurements of program executions. At the implementation-level, work is typically partitioned between a primary (master) thread and one or more worker threads. Without the OMPT interface, work executed by the master thread can be associated with its full user-level calling context and is reported under `<program root>`. However, OpenMP regions and tasks executed by worker threads typically can't be associated with the calling context in which regions or tasks were launched. Instead, the work is attributed to a worker thread outer context that polls for work, finds the work, and executes the work. HPCToolkit reports such work under `<thread root>`.

When an OpenMP runtime supports the OMPT interface, by registering callbacks using the OMPT interface and making calls to OMPT interface operations in the runtime API, HPCToolkit can gather information that enables it to reconstruct a global, user-level view of the parallelism. Using the OMPT interface, HPCToolkit can attribute metrics for costs incurred by worker threads in parallel regions back to the calling contexts in which those parallel regions were invoked. In such cases, most or all work performance is attributed back to global user-level calling contexts that are descendants of `<program root>`. When using the OMPT interface, there may be some costs that cannot be attributed back to a global user-level calling context in an OpenMP program. For instance, costs assocuated with idle worker threads that can't be associated with any parallel region may be attributed to `<omp idle>`. Even when using the OMPT interface, some costs may be attributed to `<thread root>`; however, such costs are typically small and are often associated with runtime startup.

## Monitoring OpenMP Offloading on GPUs

HPCToolkit includes support for using the OMPT interface to monitor offloading of computations specified with OpenMP TARGET to GPUs and attributing them back to the host calling contexts from which they were offloaded.

OpenMP computations executing on AMD GPUs are monitored whenever `hpcrun`'s command-line switches are configured to monitor operations on AMD, Intel, or NVIDIA GPUs, as described in Section [9.1](#sec:gpu-quickstart).

### AMD Compilers

AMD's ROCm 5.1 and later releases contains OMPT support for monitoring and attributing host computations as well as computations
offloaded to AMD GPUs using OpenMP TARGET. When compiled with `amdclang` or `amdclang++`, both host computations and computations offloaded to AMD GPUs can be associated with global user-level calling contexts that are children of `<program root>`.

Unfortunately, AMD's Rocprofiler-sdk provides only an idiosyncratic strategy for monitoring OpenMP offloading that does not follow the OpenMP standard. As a result, HPCToolkit presently shows implementation-level runtime frames in its reconstruction of global calling contexts for OpenMP TARGET regions for programs compiled with `amdclang` or `amdclang++`.

### Intel Compilers

Intel's OneAPI `ifx` and `icx` compilers, which support OpenMP offloading in their OpenMP runtime atop Intel's latest GPU-enabled Level Zero runtime, provide support for the OMPT tools interface.
The implementation of host-side OMPT callbacks in Intel's OpenMP runtime is sufficient for attributing both CPU and GPU work to global, user-level calling contexts rooted at `<program root>`. However, at this writing, Intel's runtime lacks proper OMPT support for monitoring OpenMP offloading. As a result, HPCToolkit presently shows implementation-level runtime frames in its reconstruction of global calling contexts for OpenMP TARGET regions for programs compiled with `icx` or `icpx`.

### NVIDIA Compilers

At this writing, NVIDIA's OpenMP `nvc++` runtime lacks OMPT support.
Without OMPT support, HPCToolkit separates performance information for the OpenMP primary thread from other OpenMP threads (and any other threads that may be present at
runtime, such as MPI helper threads).
Performance of the primary thread is attributed to `<program root>`; the performance of all other threads is attributed to `<thread root>`.
While this is not as easy to analyze and understand as the global, user-level calling context view constructed using the OMPT interface, this approach can be used to analyze performance data for OpenMP programs compiled with NVIDIA's compilers using HPCToolkit.

Regardless of what compiler is used to offload OpenMP computations to NVIDIA GPUs, HPCToolkit simplifies host calling contexts to which it attributes GPU operations by hiding all NVIDIA library frames that correspond to stripped code in NVIDIA's CUDA runtime.
The presence of long chains of procedure frames only identified by their machine code address in NVIDIA's CUDA library in the calling contexts for GPU operations obscures rather than enlightens; thus, suppressing them is appropriate.

### Cray Compilers

Cray's compilers `craycc` and `craycxx` have host-side support for the OMPT interface, enabling HPCToolkit to reconstruct user-level calling contexts from implementation-level measurements.

However, experiments with binaries generated by `craycxx` on an MI300A to assess runtime support for monitoring OpenMP offloading didn't interact well with AMD's Rocprofiler-sdk in module `rocm/6.4.0` which emitted the error messages shown below and failed to monitor OpenMP offloading:

```
W20251126 11:59:51.873723 140496074901440 code_object.cpp:768] No binary registered for HIP
E20251126 11:59:51.873772 140496074901440 code_object.cpp:800] hip mapping data not initialized
```

Cray's `craycxx` from module `cce/20.0.0` is incompatible with ROCM 7, which lacks the library `libamdhip64.so.6`.

### GCC Compilers

It appears that GCC's support for OpenMP offloading can only be used with `libgomp`. No experimentation was done with gcc and OpenMP offloading to see whether vendor tool libraries observe offloading.
