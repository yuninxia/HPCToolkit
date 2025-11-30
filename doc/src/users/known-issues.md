<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

# Known Issues

This section lists some known issues and potential workarounds.
Other known issues can be seen in the project's Gitlab issues pages:

- For HPCToolkit in general, see <https://gitlab.com/HPCToolkit/HPCToolkit/issues>

- For hpcviewer, see <https://gitlab.com/HPCToolkit/HPCViewer/issues>

## No support for CUDA 13

In CUDA 13.0, NVIDIA removed a deprecated API used by HPCToolkit for PC sampling, so HPCToolkit can't be compiled against CUDA 13. When compiled against CUDA 12 and run with CUDA 13, `hpcrun`'s calls to `cuFuncGetModule` fail. As a result, there is no way to use HPCToolkit with CUDA 13 at present. We recommend using CUDA 12 as a stopgap solution if you want to measure your program with HPCToolkit.

## When monitoring applications that use ROCm, using LD_AUDIT in `hpcrun` may cause it to fail to elide OpenMP runtime frames

Description:
: When an application provides a runtime that supports the OpenMP tools API known as OMPT, normally in the OpenMP runtime frames between user code on call stacks are elided. However, we have observed that when using Glibc's `LD_AUDIT` as part of HPCToolkit's measurement infrastructure in conjunction with ROCm's Rocprofiler and Roctracer, an application's TLS storage is incorrectly reinitialized during HPCToolkit's initialization; this clears some important HPCToolkit state information from thread local variables. As a result, the primary thread is not recognized as an OpenMP thread, which is necessary to elide runtime frames.

This bug was reported to Red Hat (https://sourceware.org/bugzilla/show_bug.cgi?id=31717) and fixed in Glibc 2.41, which is considerably newer than the Glibc on almost all installed systems.

Workaround:
: Use the `--disable-auditor` option to `hpcrun`.

## When using instrumentation on Intel GPUs, `hpcrun` may report that substantial time is spent in a partial call path consisting of only an unknown procedure

Description:
: Binary instrumentation on Intel GPUs uses Intel's GTPin. GTPin runs in its own private namespace. Asynchronous samples collected in response to Linux timer or hardware counter events may often occur when GTPin is executing. With GTPin in a private namespace, its code and symbols are invisible to `hpcrun`, which causes a degenerate unwind consisting of only an unknown procedure.

Workaround:
: Don't collect Linux timer or hardware counter events on the CPU when using binary instrumentation to collect instruction-level performance measurements of kernels executing on Intel GPUs.

## `hpcrun` reports partial call paths for code executed by a constructor prior to entering main

Description:
: At present, all samples of code executed by constructors are reported as a partial call paths even if they are full unwinds. This occurs because HPCToolkit wasn't designed to attribute code that executes in constructors.

Workaround:
: Don't be concerned by partial call paths that unwind through `__libc_start_main` and `__lib_csu_init`. The samples are fully attributed even though HPCToolkit does not recognize them as such.

Development Plan:
: A future version of HPCToolkit will recognize that these unwinds are indeed full call paths and attribute them as such.

## `hpcrun` may fail to measure a program execution on a CPU with hardware performance counters

Description:
: We observed a problem using Linux `perf_events` to measure CPU performance using hardware performance counters on an `x86_64` cluster at Sandia. An investigation determined that the cluster was running Sandia's LDMS (Lightweight Distributed Metric Service)---a low-overhead, low-latency framework for collecting, transferring, and storing metric data on a large distributed computer system. On this cluster, the LDMS daemon had been configured to use the `syspapi_sampler` (<https://github.com/ovis-hpc/ovis/blob/OVIS-4/ldms/src/sampler/syspapi/syspapi_sampler.c>), which uses the Linux `perf_events` subsystem to measure hardware counters at the node level. At present, the LDMS `syspapi_sampler`'s use of the Linux `perf_events` subsystem for data collection at the node level conflicts with native use of use the Linux `perf_events` subsystem by HPCToolkit for process-level measurement.

Workaround:
: Surprisingly, measurement using HPCToolkit's PAPI interface atop Linux `perf_events` works even though using HPCToolkit directly atop Linux `perf_events` yields no measurement data. For instance, rather than measuring `cycles` using Linux `perf_events` directly with `-e cycles`, one can measure cycles through HPCToolkit's PAPI measurement subsystem using `-e PAPI_TOT_CYC`. Of course, one can configure PAPI to measure other hardware events, such as graduated instructions and cache misses.

Development Plan:
: Identify why the use of the Linux `perf_events` subsystem by the LDMS `syspapi_sampler` conflicts with the use of the direct use of Linux `perf_events` HPCToolkit and the Linux `perf` tool but not with the use of Linux `perf_events` by PAPI.

## hpcrun may associate several profiles and traces with rank 0, thread 0

Description:
: On HPE systems, we have observed that `hpcrun` associates several profiles and traces with rank 0, thread 0. This results from the fact that a PMI daemon gets forked from the application in a constructor and there is no exec. Initially, each process gets tagged with rank 0, thread 0 until the real rank and thread is determined later in the execution. That determination never happens for the PMI daemon.

Workaround:
: In our experience, the hpcrun files in the measurement for the daemon tagged with rank 0 thread 0 are very small. In experiments we ran, they were about 2K. You can remove these profiles and their matching trace files before processing a measurement database with `hpcprof`. The correspondence between a profile and trace can be determined because they only differ in their suffix (hpcrun or hpctrace).

## `hpcrun` sometimes enables writing of read-only data

If an application or shared library contains a `PT_GNU_RELRO` segment in its program header, the runtime loader `ld.so` will mark all data in that segment readonly
after relocations have been processed at runtime.
As described in Section [5.1.1.1](#hpcrun-audit) of the manual, on `x86_64` and Power architectures, `hpcrun` uses `LD_AUDIT` to monitor operations on dynamic libraries.
For `hpcrun` to properly resolve calls to functions in shared libraries, the Global Offset Table (GOT) must be writable. Sometimes, the GOT lies within the `PT_GNU_RELRO` segment, which may cause it to be marked readonly after relocations are processed.
If `hpcrun` is using `LD_AUDIT` to monitor shared library operations, it will enable write permissions on the `PT_GNU_RELRO` segment during execution. While this makes some data writable that should have read-only permissions, it should not affect the behavior of any program that does not attempt to overwrite data that should have been readonly in its address space.

## A confusing label for GPU theoretical occupancy

Affected architectures:

: NVIDIA GPUs

Description:

: When analyzing a GPU-accelerated application that employs NVIDIA GPUs, HPCToolkit estimates percent GPU theoretical occupancy as the ratio of active GPU threads divided by the maximum number of GPU threads available. In multi-threaded or multi-rank programs, HPCToolkit reports GPU theoretical occupancy with the label

  ```
  Sum over rank/thread of exclusive 'GPU kernel: theoretical occupancy (FGP_ACT / FGP_MAX)'
  ```

  rather than its correct label

  ```
  GPU kernel: theoretical occupancy (FGP_ACT / FGP_MAX)
  ```

  The metric is computed correctly by summing the fine-grain parallelism used in each kernel launch across all threads and ranks and dividing it by the sum of the maximum fine-grain parallelism available to each kernel launch across all threads and ranks, and presenting the value as a percent.

Explanation:

: This metric is unlike others computed by HPCToolkit. Rather than being computed by `hpcprof`, it is computed by having `hpcviewer` interpret a formula.

Workaround:

: Pay attention to the metric value, which is computed correctly and ignore its awkward label.

Development Plan:

: Add additional support to `hpcrun` and `hpcprof` to understand how derived metrics are computed and avoid spoiling their labels.
