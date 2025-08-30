<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

# FAQ and Troubleshooting

## General Measurement Failures

### Profiling `setuid` programs

`hpcrun` uses preloaded shared libraries to initiate profiling. For this
reason, it cannot be used to profile `setuid` programs.

### Problems loading dynamic libraries

On most platforms, `hpcrun` uses Glibc's `LD_AUDIT` subsystem to monitor an application's use of dynamic
libraries. Use of `LD_AUDIT` is needed to properly track loaded libraries when a
`RUNPATH` is set in the application or libraries.
Due to known bugs in Glibc's implementation, this may cause the application to crash unexpectedly.
See Section [5.1.1.1](#hpcrun-audit) for details on the issues present and how to avoid them.

### Problems caused by `gprof` instrumentation

When an application has been compiled with the compiler flag `-pg`,
the compiler adds instrumentation to collect performance measurement data for
the `gprof` profiler. Measuring application performance with
HPCToolkit's measurement subsystem and `gprof` instrumentation
active in the same execution may cause the execution
to abort. One can detect the presence of `gprof` instrumentation in an
application by the presence of the `__monstartup` and `_mcleanup` symbols
in a executable. You can recompile your code without the
`-pg` compiler flag and measure again. Alternatively, you can use the `--disable-gprof`
argument to `hpcrun` to disable `gprof` instrumentation while
measuring performance with HPCToolkit.

To cope with `gprof` instrumentation in dynamically-linked programs, you can use `hpcrun`'s `--disable-gprof` option.

## Measurement Failures using NVIDIA GPUs

### Deadlock while monitoring a program that uses IBM Spectrum MPI and NVIDIA GPUs

IBM's Spectrum MPI uses a special library `libpami_cudahook.so` to intercept allocations of GPU memory so that Spectrum MPI knows when data is allocated on an NVIDIA GPU.
Unfortunately, the mechanism used by Spectrum MPI to do so (wrapping `dlsym`) interferes with performance tools that use `dlopen` and `dlsym`.
This interference causes measurement of a GPU-accelerated MPI application using HPCToolkit to deadlock when an application uses both Spectrum MPI and CUDA on an NVIDIA GPU while not using `hpcrun`'s `LD_AUDIT` support. `LD_AUDIT` support is typically enabled, although on some platforms (e.g. Aurora), it is not. If `LD_AUDIT` is disabled, it can be enabled using `--enable-auditor`.

If `LD_AUDIT` cannot be used, e.g. because an application uses `dlmopen` which causes `LD_AUDIT` to fail prior to glibc 2.35,
when launching a program that uses Spectrum MPI with `jsrun`, one can use `--smpiargs="-x PAMI_DISABLE_CUDA_HOOK=1 -disable_gpu_hooks"` to disable the PAMI CUDA hook library.
These flags cannot be used with the `-gpu` flag.

Note however that disabling Spectrum MPI's CUDA hook will cause trouble if CUDA device memory is passed into the MPI library as a send or receive buffer.
An additional restriction is that memory obtained with a call to `cudaMallocHost` may not be passed as a send or receive buffer.
Functionally similar memory may be obtained with any host allocation function followed by a call the `cudaHostRegister`.

### Ensuring permission to use GPU performance counters

Your Administrator or a recent NVIDIA driver installation may have disabled access to GPU Performance due to Security Notice: NVIDIA Response to "Rendered Insecure: GPU Side Channel Attacks are Practical" <https://nvidia.custhelp.com/app/answers/detail/a_id/4738> - November 2018.
If that is the case, HPCToolkit cannot access NVIDIA GPU performance counters when using a Linux 418.43 or later driver. This may cause an error message when you try to use PC sampling on an NVIDIA GPU.

A good way to check whether GPU performance counters are available to non-root users on Linux is to execute the following commands:

1. `cd /etc/modprobe.d`

1. `grep NVreg_RestrictProfilingToAdminUsers *`

Generally, if non-root user access to GPU performance counters is enabled, the `grep` command above should yield a line that contains `NVreg_RestrictProfilingToAdminUsers=0`.
Note: if you are on a cluster, access to GPU performance counters may be disabled on a login node, but enabled on a compute node. You should run an interactive job on a compute node and perform the checks there.

If access to GPU hardware performance counters is not enabled, one option you have is to use `hpcrun` without PC sampling, i.e., with the `-e gpu=nvidia` option instead of `-e gpu=nvidia,pc`.

If PC sampling is a must, you have two options:

1. Run the tool or application being profiled with administrative privileges.
   On Linux, launch HPCToolkit with `sudo` or as a user with the `CAP_SYS_ADMIN` capability set.

1. Have a system administrator enable access to the NVIDIA performance counters using the instructions on the following web page: <https://developer.nvidia.com/ERR_NVGPUCTRPERM>.

### Avoiding the error `cudaErrorUnknown`

When monitoring a CUDA application with `REALTIME` or `CPUTIME`, you may encounter a
`cudaErrorUnknown` return from many or all CUDA calls in the application.[^18]
This error may occur non-deterministically. We have observed that this error occurs regularly
at very fast periods such as `REALTIME@100`. If this occurs, we recommend using `CYCLES`
as a working alternative similar to `CPUTIME`, see Section [12.4.1](#sec:troubleshooting:hpcrun-sample-periods)
for more detail on HPCToolkit's `perf_events` support.

### Avoiding the error `CUPTI_ERROR_NOT_INITIALIZED`

`hpcrun` uses NVIDIA's CUDA Performance Tools Interface known as
CUPTI to monitor computations on NVIDIA GPUs. In our experience,
this error occurs when the version of CUPTI used by HPCToolkit is
incompatible with the version of CUDA used by your program or CUDA kernel driver installed on your system. You can check the
version of the CUDA kernel driver installed on your system using the `nvidia-smi` command.
Table 3 *CUDA Application Compatibility Support Matrix* at the following URL <https://docs.nvidia.com/cuda/cuda-toolkit-release-notes>
specifies what versions of the CUDA kernel driver match each version of CUDA and CUPTI.
Although the table indicates that some drivers can support newer versions of CUDA than the one that they were designed for,
in our experience that does not necessarily mean that the driver will support performance measurement of CUDA programs using a newer CUPTI.
We believe that best way to avoid the `CUPTI_ERROR_NOT_INITIALIZED` error is to ensure that
(1) HPCToolkit is compiled with the version of CUDA that your installed CUDA kernel driver was designed to support, and
(2) your application uses the version of CUDA that matches the one your kernel driver was designed to support or a compatible older version.

### Avoiding the error `CUPTI_ERROR_HARDWARE_BUSY`

When trying to use PC sampling to measure computation on an NVIDIA GPU, you may encounter the following error: 'function `cuptiActivityConfigurePCSampling` failed with error `CUPTI_ERROR_HARDWARE_BUSY`'.

For all versions of CUDA to date (through CUDA 11), NVIDIA's CUPTI library only supports PC sampling for only one process per GPU. If multiple MPI ranks in your application run CUDA on the same GPU, you may see this error.

You have two alternatives:

1. Measure the execution in which multiple MPI ranks share a GPU using only `-e gpu=nvidia` without PC sampling.

1. Launch your program so that there is only a single MPI rank per GPU.

   1. `jsrun` advice: if using `-g1` for a resource set, don't use anything other than `-a1`.

### Avoiding the error `CUPTI_ERROR_UNKNOWN`

When trying to use PC sampling to measure computation on an NVIDIA GPU, you may encounter the following error: 'function `cuptiActivityEnableContext` failed with error `CUPTI_ERROR_UNKNOWN`'.

For all versions of CUDA to date (through CUDA 11), NVIDIA's CUPTI library only supports PC sampling for only one process per GPU. If multiple MPI ranks in your application run CUDA on the same GPU, you may see this error. You have two alternatives:

1. Measure the execution in which multiple MPI ranks share a GPU using only `-e gpu=nvidia` without PC sampling.

1. Launch your program so that there is only a single MPI rank per GPU.

   1. `jsrun` advice: if using `-g1` for a resource set, don't use anything other than `-a1`.

## General Measurement Issues

(sec:troubleshooting:hpcrun-sample-periods)=

### How do I choose sampling periods?

When using sample sources for hardware counter and software counter events provided by Linux `perf_events`,
we recommend that you use frequency-based sampling. The default frequency is 300 samples/second.

Statisticians use samples sizes of approximately 3500 to make accurate projections about the voting preferences of millions of people.
In an analogous way, rather than measuring and attributing every action of a program or every runtime event (e.g., a cache miss), sampling-based performance measurement collects "just enough" representative performance data.
You can control `hpcrun`'s sampling periods to collect "just enough" representative data even for very long executions and, to a lesser degree, for very short executions.

For reasonable accuracy (+/- 5%), there should be at least 20 samples in each context that is important with respect to performance.
Since unimportant contexts are irrelevant to performance, as long as this condition is met (and as long as samples are not correlated, etc.), HPCToolkit's performance data should be accurate enough to guide program tuning.

We typically recommend targeting a frequency of hundreds of samples per second.
For very short runs, you may need to collect thousands of samples per second to record an adequate number of samples.
For long runs, tens of samples per second may suffice for performance diagnosis.

Choosing sampling periods for some events, such as Linux timers, cycles and instructions, is easy given a target sampling frequency.
Choosing sampling periods for other events such as cache misses is harder.
In principle, an architectural expert can easily derive reasonable sampling periods by working backwards from (a) a maximum target sampling frequency and (b) hardware resource saturation points.
In practice, this may require some experimentation.

See also the `hpcrun` [man page](https://hpctoolkit.org/man/hpcrun.html).

### Why do I see partial unwinds?

Under certain circumstances, HPCToolkit can't fully unwind the call
stack to determine the full calling context where a sample event
occurred. Most often, this occurs when `hpcrun` tries to unwind
through functions in a shared library or executable that has not
been compiled with `-g` as one of its options. The `-g`
compiler flag can be used in addition to optimization flags. On
Power and `x86_64` processors, `hpcrun` can often compensate
for the lack of unwind recipes by using
binary analysis to compute recipes itself. However, since `hpcrun`
lacks binary analysis capabilities for ARM processors, there is a
higher likelihood that the lack of a `-g` compiler option for
an executable or a shared library will lead to partial unwinds.

One annoying place where partial unwinds are somewhat common on `x86_64`
processors is in Intel's MKL family of libraries. A careful examination of Intel's
MKL libraries showed that most but not all routines have
compiler-generated Frame Descriptor Entries (FDEs) that help tools
unwind the call stack. For any routine that lacks
an FDE, HPCToolkit tries to compensate using binary analysis.
Unfortunately, highly-optimized code in MKL library
routines has code features that are difficult to analyze correctly.

There are two ways to deal with this problem:

- Analyze the execution using information from partial unwinds. Often knowing several levels of calling context is enough for analysis without full calling context for sample events.

- Recompile the binary or shared library causing the problem and add `-g` to the list of its compiler options.

### Measurement with HPCToolkit has high overhead! Why?

For reasonable sampling periods, we expect `hpcrun`'s overhead percentage to be in the low single digits, e.g., less than 5%.
The most common causes for unusually high overhead are the following:

- Your sampling frequency is too high.
  Recall that the goal is to obtain a representative set of performance data.
  For this, we typically recommend targeting a frequency of hundreds of samples per second.
  For very short runs, you may need to try thousands of samples per second.
  For very long runs, tens of samples per second can be quite reasonable.
  See also Section [12.4.1](#sec:troubleshooting:hpcrun-sample-periods).

- `hpcrun` has a problem unwinding.
  This causes overhead in two forms.
  First, `hpcrun` will resort to more expensive unwind heuristics and possibly have to recover from self-generated segmentation faults.
  Second, when these exceptional behaviors occur, `hpcrun` writes some information to a log file.
  In the context of a parallel application and overloaded parallel file system, this can perturb the execution significantly.
  To diagnose this, you can grep the log files in a measurement directory for large counts of "Errant Samples", which appear after the string "errant:".

- You have very long call paths where long is in the hundreds or thousands.
  On x86-based architectures, try additionally using `hpcrun`'s `RETCNT` event.
  This has two effects: It causes `hpcrun` to collect function return counts and to memoize common unwind prefixes between samples.

- Currently, on very large runs the process of writing profile data can take a long time.
  However, because this occurs after the application has finished executing, it is relatively benign overhead.
  (We plan to address this issue in a future release.)

- At runtime, `hpcrun` analyzes CPU binaries loaded into an application's address space.
  This analysis occurs when libraries are loaded. Most libraries are loaded at program launch.
  This analysis might take seconds for your program. For short-running programs, this can lead to high overhead.
  However, time for this analysis is not actually considered part of the execution time measured by `hpcrun`.

### Some of my syscalls return EINTR

When profiling a threaded program, there are times when it is
necessary for `hpcrun` to signal another thread to take some action.
When this happens, if the thread receiving the signal is blocked in a
syscall, the kernel may return EINTR from the syscall. This would
happen only in a threaded program and mainly with "slow" syscalls
such as `select()`, `poll()` or `sem_wait()`.

### My application spends a lot of time in C library functions with names that include `mcount`

If performance measurements with HPCToolkit show that your
application is spending a lot of time in C library routines with
names that include the string `mcount` (e.g., `mcount`, `_mcount` or
`__mcount_internal`), your code has been compiled with the compiler
flag `-pg`, which adds instrumentation to collect performance
measurement data for the `gprof` profiler. If you are using
HPCToolkit to collect performance data, the `gprof` instrumentation
is needlessly slowing your application. You can recompile your code without the
`-pg` compiler flag and measure again. Alternatively, you can use the `--disable-gprof`
argument to `hpcrun` to disable `gprof` instrumentation while
measuring performance with HPCToolkit.

(section:hpcstruct-cubin)=

## Problems Recovering Loops in NVIDIA GPU binaries

- When using the `--gpucfg yes` option to analyze control flow to recover information about loops in CUDA binaries, `hpcstruct` needs to use NVIDIA's `nvdisasm` tool. It is important to note that `hpcstruct` uses the version of `nvdisasm` that is on your path. When using the `--gpucfg yes` option to recover loops in CUBINs, you can improve `hpcstruct`'s ability to recover loops by having a newer version of `nvdisasm` on your path. Specifically, the version of `nvdisasm` in CUDA 11.2 is much better than `nvdisasm` in CUDA 10.2. It will recover loops for more procedures and faster.

- While NVIDIA has improved the capability and speed of `nvdisasm` in CUDA 11.2, it may still be too slow to be usable on large CUDA binaries. Because of failures we have encountered with `nvdisasm`, `hpcstruct` launches `nvdisasm` once for each procedure in a GPU binary to maximize the information it can extract. With this approach, we have seen `hpcstruct` take over 12 hours to analyze a CUBIN of roughly 800MB with 40K GPU functions. For large CUDA binaries, our advice is to skip the `--gpucfg yes` option at present until we adjust `hpcstruct` launch multiple copies of `nvdisasm` in parallel to reduce analysis time.

## Graphical User Interface Issues

### `hpcviewer` fails to launch

`hpcviewer` saves settings from your preferences. Typically, this information is recorded in `$HOME/.hpctoolkit/hpcviewer`. Often, removing this directory and relaunching `hpcviewer` will solve this problem. A future version of `hpcviewer` will tag recorded state with a version number, gracefully fail, and alert you to the mismatch. With this approach, you will have a choice to use a version of `hpcviewer` that matches your saved state or remove the saved state so that you can use a different version of `hpcviewer`.

### Fail to run `hpcviewer`: executable launcher was unable to locate its companion shared library

Although this error mostly incurrs on Windows platform, but it can happen in other environment.
The cause of this issue is that the permission of one of Eclipse launcher library (org.eclipse.equinox.launcher.\*) is too restricted.
To fix this, set the permission of the library to 0755, and launch again the viewer.

### Launching `hpcviewer` is very slow on Windows

There is a known issue that Windows Defender significantly slow down Java-based applications.
See the github issue at <https://github.com/microsoft/java-wdb/issues/9>.

A temporary solution is to add `hpcviewer` in the Windows' exclusion list:

1. Open Windows settings.

1. Search for "Virus and threat protection" and open it.

1. Now click on "Manage settings" under "Virus and threat protection settings" section.

1. Now click "Add or remove exclusions" under "Exclusions" section.

1. Now click "Add an exclusion" then select "Folder"

1. Point to `hpcviewer` directory and press "Select Folder"

### Mac only: `hpcviewer` runs on Java X instead of "Java 17"

`hpcviewer` has mainly been tested on Java versions 17 and 21. If you are running an older than Java 17 or newer than Java 21, obtain a version of Java 17 or 21 from <https://adoptium.net/>.

If your system has multiple versions of Java and Java 17 or 21 is not the newest version, you need to set Java 17 or 21 as the default JVM. On MacOS, you need to exclude older Java as follows:

1. Leave all JDKs at their default location (usually under `/Library/Java/JavaVirtualMachines`). The system will pick the highest version by default.

1. To exclude a JDK from being picked by default, rename `Contents/Info.plist` file to other name like `Info.plist.disabled`. That JDK can still be used when `$JAVA_HOME` points to it, or explicitly referenced in a script or configuration. It will simply be ignored by your Mac's java command.

### When executing `hpcviewer`, it complains cannot create "Java Virtual Machine"

If you encounter this problem, we recommend that you edit the
`hpcviewer.ini`
file which is located in HPCToolkit installation directory to reduce the
Java heap size.
By default, the content of the file on Linux `x86` for `hpcviewer 2025.02` is as follows:

```
-startup
plugins/org.eclipse.equinox.launcher_1.6.800.v20240513-1750.jar
--launcher.library
plugins/org.eclipse.equinox.launcher.gtk.linux.x86_64_1.2.1000.v20240506-2123
-clearPersistedState
-vmargs
-Xmx8G
-Dosgi.locking=none
-Dslf4j.provider=ch.qos.logback.classic.spi.LogbackServiceProvider
-Dosgi.requiredJavaVersion=17

```

You can decrease the maximum size of the Java heap from 8G to 2GB
by changing the `Xmx` specification in the `hpcviewer.ini` file as follows:

```
-Xmx2GB
```

### `hpcviewer` fails to launch due to `java.lang.NoSuchMethodError` exception.

The root cause of the error is due to a mix of old and new hpcviewer binaries.
To solve this problem, you need to remove your hpcviewer workspace (usually in your `$HOME/.hpctoolkit/hpcviewer` directory), and run `hpcviewer` again.

### `hpcviewer` fails due to `java.lang.OutOfMemoryError` exception.

If you see this error, the memory footprint that `hpcviewer` needs to store and the metrics for your measured program execution exceeds the maximum size for the Java heap specified at program launch. On Linux, `hpcviewer` accepts a command-line option `--java-heap` that enables you to specify a larger non-default value for the maximum size of the Java heap. Run `hpcviewer --help` for the details of how to use this option.

### `hpcviewer` writes a long list of Java error messages to the terminal!

The Eclipse Java framework that serves as the foundation for `hpcviewer` can be somewhat temperamental. If the persistent state maintained by Eclipse for `hpcviewer`
gets corrupted, `hpcviewer` may spew a list of errors deep within call chains of the Eclipse framework.

On MacOS and Linux, try removing your `hpcviewer` Eclipse workspace with default location `$HOME/.hpctoolkit/hpcviewer` and run `hpcviewer` again.

(sec:troubleshooting:debug-info)=

### `hpcviewer` attributes performance information only to functions and not to source code loops and lines! Why?

Most likely, your application's binary either lacks debugging information or is stripped.
A binary's (optional) debugging information includes a line map that is used by profilers and debuggers to map object code to source code.
HPCToolkit can profile binaries without debugging information, but without such debugging information it can only map performance information (at best) to functions instead of source code loops and lines.

For this reason, we recommend that you always compile your production applications with optimization *and* with debugging information.
The options for doing this vary by compiler.
We suggest the following options:

- GNU compilers (`gcc`, `g++`, `gfortran`): `-g`

- IBM compilers (xlc, xlf, xlC): `-g`

- Intel compilers (`icc`, `icpc`, `ifort`): `-g -debug inline_debug_info`.

We generally recommend adding optimization options *after* debugging options --- e.g., '`-g -O2`' --- to minimize any potential effects of adding debugging information.
Also, be careful not to strip the binary as that would remove the debugging information.
(Adding debugging information to a binary does not make a program run slower; likewise, stripping a binary does not make a program run faster.)

Please note that at high optimization levels, a compiler may make significant program transformations that do not cleanly map to line numbers in the original source code.
Even so, the performance attribution is usually very informative.

### `hpcviewer` hangs trying to open a large database! Why?

The most likely problem is that the Java virtual machine is low on memory and thrashing. The memory footprint that `hpcviewer` needs to store and the metrics for your measured program execution is likely near the maximum size for the Java heap specified at program launch.

On Linux, `hpcviewer` accepts a command-line option `--java-heap` that enables you to specify a larger non-default value for the maximum size of the Java heap. Run `hpcviewer --help` for the details of how to use this option.

### `hpcviewer` runs glacially slowly! Why?

There are three likely reasons why `hpcviewer` might run slowly.
First, you may be running `hpcviewer` on a remote system with low bandwidth, high latency or an otherwise unsatisfactory network connection to your desktop.
If any of these conditions are true, `hpcviewer`'s otherwise snappy GUI can become sluggish if not downright unresponsive.
The solution is to install `hpcviewer` on your local system, copy the database onto your local system, and run `hpcviewer` locally.
We almost always run `hpcviewer` on our local desktops or laptops for this reason.

Second, the HPCToolkit database may be very large, which can cause the Java virtual machine to run short on memory and thrash.
The memory footprint that `hpcviewer` needs to store and the metrics for your measured program execution is likely near the maximum size for the Java heap specified at program launch.
On Linux, `hpcviewer` accepts a command-line option `--java-heap` that enables you to specify a larger non-default value for the maximum size of the Java heap. Run `hpcviewer --help` for the details of how to use this option.

### `hpcviewer` does not show my source code! Why?

Assuming you compiled your application with debugging information (see Issue [12.6.8](#sec:troubleshooting:debug-info)),
the most common reason that `hpcviewer` does not show source code is that `hpcprof/mpi`
could not find it and therefore could not copy it into the HPCToolkit performance database.

#### An explanation how HPCToolkit finds source files

`hpcprof/mpi` obtains source file names from your application binary's debugging information.
If debugging information is unavailable, such as is often the case for system or math libraries, then source files are unknown.

Two things immediately follow from this.
First, in most normal situations, there will always be some functions for which source code cannot be found, such as those within system libraries.[^19]
Second, to ensure that `hpcprof/mpi` has file names for which to search, make sure as much of your application as possible (including libraries) contains debugging information.

If debugging information is available, source files can come in two forms: absolute and relative.
`hpcprof/mpi` can find source files under the following conditions:

- If a source file path is absolute and the source file can be found on the file system, then `hpcprof/mpi` will find it.

- If a source file path is relative, `hpcprof/mpi` can only find it if the source file can be found from the current working directory.

- Finally, if a source file path is absolute and cannot be found by its absolute path, `hpcprof/mpi` uses a special search mode.
  Let the source file path be `p/f`.
  If the path's base file name `f` is found within a search directory, then that is considered a match.
  This special search mode accommodates common complexities such as:
  (1) source file paths that are relative not to your source code tree but to the directory where the source was compiled;
  (2) source file paths to source code that is later moved; and
  (3) source file paths that are relative to file system that is no longer mounted.

Note that given a source file path `p/f` (where `p` may be relative or absolute), it may be the case that there are multiple instances of a file's base name `f` within one search directory, e.g., `p_1/f` through `p_n/f`, where `p_i` refers to the `i`th path to `f`.
Similarly, with multiple search-directory arguments, `f` may exist within more than one search directory.
If this is the case, the source file `p/f` is resolved to the first instance `p'/f` such that `p'` best corresponds to `p`, where instances are ordered by the order of search directories on the command line.

For any functions whose source code is not found (such as functions within system libraries), `hpcviewer` will generate a synopsis that shows the presence of the function and its line extents (if known).

Hypothetically, let's say that your HPCToolkit database is missing source code from PetSC and you linked your program against a copy of PetSC provided by a module provided by your system administrators. You can check if that library contains line map information by running `readelf --debug-dump=decodedline`. In the `readelf` output, if you see that the source file paths begin with `/path/to/petsc`, then you can download a matching version of PetSC to a location of your choosing `/my/path/to/petsc`. Then, you can rerun `hpcprof/mpi` with the `-R` option, which is used to replace path prefixes when searching for source files. In this example, you would use `-R /path/to/petsc=/my/path/to/petsc` to instruct `hpcprof/mpi` to consider all path prefixes of `/path/to/petsc` as `/my/path/to/petsc` so `hpcprof/mpi` will find the copies of source that you downloaded.

### `hpcviewer`'s reported line numbers do not exactly correspond to what I see in my source code! Why?

To use a cliché, "garbage in, garbage out".
HPCToolkit depends on information recorded in the symbol table by the compiler.
Line numbers for procedures and loops are inferred by looking at the symbol table information recorded for machine instructions identified as being inside the procedure or loop.

For procedures, often no machine instructions are associated with a procedure's declarations. In that case, thae function might be mapped back to the first statement in the function that had machine code associated with it.

Inlined functions may occasionally lead to confusing data for a procedure.
Machine instructions mapped to source lines from the inlined function appear in the context of other functions.
While `hpcprof`'s methods for handling incline functions are good, some codes can confuse the system.

For loops, the process of identifying what source lines are in a loop is similar to the procedure process: what source lines map to machine instructions inside a loop defined by a backward branch to a loop head. For some compilers, that may cause a loop to be mapped back to its closing brace rather than beginning of the loop.

### `hpcviewer` claims that there are several calls to a function within a particular source code scope, but my source code only has one! Why?

In the course of code optimization, compilers often replicate code blocks.
For instance, as it generates code, a compiler may peel iterations from a loop or split the iteration space of a loop into two or more loops.
In such cases, one call in the source code may be transformed into multiple distinct calls that reside at different code addresses in the executable.

When analyzing applications at the binary level, it is difficult to determine whether two distinct calls to the same function that appear in the machine code were derived from the same call in the source code.
Even if both calls map to the same source line, it may be wrong to coalesce them; the source code might contain multiple calls to the same function on the same line.
By design, HPCToolkit does not attempt to coalesce distinct calls to the same function because it might be incorrect to do so; instead, it independently reports each call site that appears in the machine code.
If the compiler duplicated calls as it replicated code during optimization, multiple call sites may be reported by `hpcviewer` when only one appeared in the source code.

### hpcviewer's Trace view shows lots of white space on the left. Why?

At startup, Trace view renders traces for the time interval between the minimum and maximum times recorded for any process or thread in the execution. The minimum time for each process or thread is recorded when its trace file is opened as HPCToolkit's monitoring facilities are initialized at the beginning of its execution. The maximum time for a process or thread is recorded when the process or thread is finalized and its trace file is closed. When an application uses the `hpctoolkit_start` and `hpctoolkit_stop` primitives, the minimum and maximum time recorded for a process/thread are at the beginning and end of its execution, which may be distant from the start/stop interval. This can cause significant white space to appear in Trace view's display to the left and right of the region (or regions) of interest demarcated in an execution by start/stop calls.

## Debugging

### How do I debug HPCToolkit's measurement?

Assume you want to debug HPCToolkit's measurement subsystem when
collecting measurements for an application named `app`.

### Tracing HPCToolkit's Measurement Subsystem

Broadly speaking, there are two levels at which a user can test `hpcrun`.
The first level is tracing `hpcrun`'s application control, that is, running `hpcrun` without an asynchronous sample source.
The second level is tracing `hpcrun` with a sample source.
The key difference between the two is that the former uses the `--event NONE` or `HPCRUN_EVENT_LIST="NONE"` option (shown below) whereas the latter does not (which enables the default CPUTIME sample source).
With this in mind, to collect a debug trace for either of these levels, use commands similar to the following:

```
[<mpi-launcher>] \
  hpcrun --monitor-debug --dynamic-debug ALL --event NONE \
    app [app-arguments]
```

Note that the `*debug*` flags are optional.
The `--monitor-debug/MONITOR_DEBUG` flag enables `libmonitor` tracing.
The `--dynamic-debug/HPCRUN_DEBUG_FLAGS` flag enables `hpcrun` tracing.

### Using a debugger to inspect an execution being monitored by HPCToolkit

If HPCToolkit has trouble monitoring an application, you may find it useful to
execute an application being monitored by HPCToolkit under the control
of a debugger to observe how HPCToolkit's measurement subsystem interacts with the application.

HPCToolkit's measurement subsystem is easiest to debug if you configure and
build HPCToolkit for debugging when building by Spack or Meson. See the Spack and Meson sections in this manual for how to configure debugging for your build. Note: if configuring HPCToolkit for debugging using Spack, you probably want to install hpctoolkit with the `--keep-stage` option, which instructs Spack not to remove source code (e.g. a copy of HPCToolkit) after compiling it.

One can debug a dynamically-linked applications being measured by
HPCToolkit's measurement subsystem. For a single-process program, you can use `gdb hpcrun`, set breakpoints inside `hpcrun`'s code where you want them, and then launch the application you are measuring with the gdb `run` command.

\:::\{important}
`hpcrun` launches the program it is measuring with `exec`. As a result, in `gdb` before you issue the `run` command again, you will need to use the `gdb` command `exec-file hpcrun` to tell `gdb` that you want to launch `hpcrun` with the `run` command and not the application launched by `hpcrun` using `exec`. If you forget, you will find that none of the breakpoints in `hpcrun` will be encountered and your application will run to completion unmonitored.
\:::

Alternatively, you can launch an application directly with `hpcrun` and pass the `--debug` flag on its command line. Then, from a different terminal, you can attach a debugger to the copy of your application which will be spin waiting for you to attach.

To debug hpcrun with a debugger use the following approach.

1. Launch your application.
   To debug `hpcrun` without controlling sampling signals, launch normally.
   To debug `hpcrun` with controlled sampling signals, launch as follows:

   ```
   hpcrun --debug --event REALTIME@0 app [app-arguments]
   ```

1. Attach a debugger.
   The debugger should be spinning in a loop whose exit is conditioned by the
   `HPCRUN_DEBUGGER_WAIT` variable.

1. Set any desired breakpoints.
   To send a sampling signal at a particular point, make sure to stop at that point with a *one-time* or *temporary* breakpoint (`tbreak` in GDB).

1. Call `(void) hpcrun_continue()` or set the `HPCRUN_DEBUGGER_WAIT` variable to 0
   and continue.

1. To raise a controlled sampling signal, raise a SIGPROF, e.g., using GDB's command `signal SIGPROF`.

[^18]: We have observed this error on ORNL's Summit machine, running Red Hat Enterprise Linux 8.2.

[^19]: Having a system administrator download the associated `devel` package for a library can enable visibility into the source code of system libraries.
