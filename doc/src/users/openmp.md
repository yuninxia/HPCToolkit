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

As of this writing, a popular OpenMP runtime that lacks OMPT support is the GCC compiler suite's `libgomp`. Fortunately, the LLVM OpenMP runtime, which supports OMPT, is compatible with `libgomp`, at least on the host.

````{tip}
For applications not using OpenMP's TARGET construct, one can typically use LLVM's ABI-compatible OpenMP runtime (`libomp.so`) as a replacement for `libgomp` by (1) creating a link to LLVM's `libomp` runtime whose name matches the `libgomp` dependence in your binary, and (2) putting the directory containing the link on your `LD_LIBRARY_PATH` as shown below:
```bash
ln -s /path/to/libomp.so libgomp.so.1
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```
````

In OpenMP implementations without support for OMPT, HPCToolkit records and reports implementation-level measurements of program executions. At this level, work is typically partitioned between a primary (master) thread and one or more worker threads. Work executed on the master thread can be associated with its full user-level calling context and is reported under `<program root>`. However, OpenMP regions and tasks executed by worker threads typically can't be associated with the calling context in which regions or tasks were launched. Instead, this work is attributed to a worker thread outer context that polls for work, finds the work, and executes the work. HPCToolkit reports such work under `<thread root>`.

When an OpenMP runtime supports OMPT, HPCToolkit can gather information that enables it to reconstruct a global, user-level view of the parallelism. Using OMPT, HPCToolkit can attribute performance for parallel regions incurred by worker threads back to the calling contexts in which those parallel regions were invoked. In such cases, most or all work performance is attributed back to global user-level calling contexts that are descendants of `<program root>`.

```{note}
Even when an OpenMP runtime supports OMPT, there may be some costs that cannot be attributed back to a global user-level calling context in an OpenMP program. For instance, costs associated with idle worker threads (that can't be associated with any parallel region) may be attributed to `<omp idle>`, and small costs associated with runtime startup may be attributed to `<thread root>`.
```

## Monitoring OpenMP Offloading on GPUs

HPCToolkit can measure computation offloaded onto GPUs via an OpenMP TARGET construct and attribute performance for offloaded computation back to the host calling context that launched it. This functionality is enabled when

1. The OpenMP runtime supports OMPT, and
1. The appropriate GPU monitoring flags are passed to `hpcrun` as described in Section [9.1](#sec:gpu-quickstart).

The sections below detail known support and quirks for vendor implementations of OpenMP.

### AMD OpenMP

The OpenMP runtime included with AMD's ROCm 5.1 and later releases contains OMPT support for monitoring and attributing host computations as well as computations offloaded to AMD GPUs using OpenMP TARGET. This support is available for applications compiled with the `amdclang` and `amdclang++` compilers included in ROCm.

```{note}
As of writing, AMD only provides an idiosyncratic strategy for reporting offloaded OpenMP computations that does not follow the OpenMP standard. As a result, calling contexts for computations offloaded onto AMD GPUs using OpenMP TARGET may include implementation details of AMD's OpenMP and ROCm runtimes.
```

### Intel OneAPI OpenMP

Intel's OneAPI `ifx`, `icx`, and `icpx` compilers and runtime support OpenMP offloading atop Intel's latest GPU-enabled Level Zero runtime. The OneAPI OpenMP runtime provides support for the OMPT tools interface.

The implementation of host-side OMPT callbacks in the OneAPI OpenMP runtime is sufficient for attributing both CPU and GPU work to global, user-level calling contexts rooted at `<program root>`. However, at this writing, the OneAPI OpenMP runtime lacks up-to-date OMPT support for monitoring OpenMP offloading. As a result, calling contexts for computations offloaded onto Intel GPUs using OpenMP TARGET may include implementation details of OneAPI OpenMP and Intel's Level Zero runtime.

### NVIDIA OpenMP

As of writing, NVIDIA's OpenMP runtime for its CUDA and HPC SDK compilers (`nvc`, `nvcc`, `nvc++`, `nvfortran`) lacks support for OMPT. Without OMPT support, HPCToolkit attributes performance measurements of GPU computations offloaded using OpenMP TARGET to implementation-level calling contexts.

### HPE's Cray OpenMP

HPE's Cray compilers `craycc`, `craycxx`, and `crayftn` have host-side support for the OMPT interface, which enables HPCToolkit to reconstruct user-level calling contexts from implementation-level measurements.

We have not investigated whether the implementation of OpenMP offloading for NVIDIA GPUs in the Cray OpenMP runtime supports the OMPT interface.

As of this writing, AMD's Rocprofiler-sdk doesn't report the performance of computations offloaded onto AMD GPUs by the Cray OpenMP runtime.

### GCC OpenMP

It appears that GCC's support for OpenMP offloading can only be used with `libgomp`. Since we lacked GCC compilers configured for OpenMP offloading on the systems we are using, we didn't experiment with with GCC's OpenMP offloading to see whether vendor tool libraries observe GPU computations offloaded by GCC.
