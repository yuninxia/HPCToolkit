// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

// -*-Mode: C++;-*-

#include "../common/hpctoolkit-version.h"
#include "hpcrun-sonames.h"

#include <cstdlib>
#include <deque>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <link.h>
#include <optional>
#include <sstream>
#include <string_view>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static const char diemsg[] = "\nuse 'hpcrun -h' for a summary of options\n";
static void usage() {
  std::cout <<
R"==(Usage:
  hpcrun [profiling-options] <command> [command-arguments]
  hpcrun [info-options]

hpcrun profiles the execution of an arbitrary command <command> using
statistical sampling (rather than instrumentation).  It collects
per-thread call path profiles that represent the full calling context of
sample points.  Sample points may be generated from multiple simultaneous
sampling sources.  hpcrun profiles complex applications that use forks,
execs, threads, and dynamic linking/unlinking; it may be used in conjunction
with parallel process launchers such as MPICH's mpiexec and SLURM's srun.

Note that hpcrun is unable to profile static executables.

To configure hpcrun's sampling sources, specify events and periods using
the -e/--event option.  For an event 'e' and period 'p', after every 'p'
instances of 'e', a sample is generated that causes hpcrun to inspect the
and record information about the monitored <command>.

When <command> terminates, a profile measurement database will be written to
the directory:
  hpctoolkit-<command>-measurements[-<jobid>]
where <jobid> is a job launcher id that associated with the execution, if any.

hpcrun enables a user to abort a process and write the partial profiling
data to disk by sending a signal such as SIGINT (often bound to Ctrl-C).
This can be extremely useful on long-running or misbehaving applications.

Options: Informational
  -l, -L --list-events List available events. (N.B.: some may not be profilable)
  -V, --version        Print version information.
  -h, --help           Print help.

Options: Profiling (Defaults shown in curly brackets {})
  -e <event>[@<howoften>], --event <event>[@<howoften>]
                       <event> may be a Linux system timer (CPUTIME and REALTIME),
                       a specifier for monitoring GPU operations, software or hardware
                       counter events supported by Linux perf, hardware counter events
                       supported by the PAPI library, and more. A complete list of
                       available events can be obtained by running 'hpcrun -L'. The '-e'
                       option may be given multiple times to profile several events at
                       once. If the value for <howoften> is a number, it will be interpreted
                       as a sample period. For Linux perf events, one may specify a sampling
                       frequency for 'howoften' by writing f before a number.  For
                       instance, to sample an event 100 times per second,  specify
                       <howoften>  as '@f100'. For Linux perf events, if no value for
                       <howoften> is specified, hpcrun will monitor the event using
                       frequency-based sampling at 300 samples/second. If no event is
                       specified, hpcrun will collect CPUTIME samples 200 times per
                       second per application thread.

  -a, --attribution <method>
                       Use the given <method> to produce the calling contexts that metrics
                       are attributed to. By default, the calling context is produced by
                       unwinding C/C++ call stack. Passing this option allows for
                       additive or alternative methods to be used, multiple options may be
                       passed to apply multiple modifications. Available methods:

                           -a flat
                                Don't unwind the call stack, instead attribute metrics to
                                the leaf procedures (i.e. produce a "flat" profile).
)=="
#ifdef ENABLE_LOGICAL_PYTHON
R"==(
                           -a python
                                Attribute to Python code and functions, instead of to
                                C functions in the Python interpreter.
                                NOTE: May cause crashes or not function if used with a
                                different Python than HPCToolkit was built with.
                                Highly experimental. Use at your own risk.
)=="
#endif
R"==(
  -c, --count <howoften>
                       Only  available  for  events  managed  by Linux perf. This
                       option specifies a default value for how often to sample. The
                       value for <howoften> may be a number that will be used as a
                       default event period or an f followed by a number, e.g. f100,
                       to specify a default sampling frequency in samples/second.

  -t, --trace          Generate a call path trace in addition to a call
                       path profile.

  -tt, --ttrace        Enhanced resolution tracing. Generate a call path trace that
                       includes both sample and kernel launches on the CPU in
                       addition to a call path profile. However, since `-tt` collects
                       samples at an irregular rate, statistical properties of the
                       CPU traces are disturbed and cannot be trusted when this flag
                       is used.

  --omp-serial-only    When profiling using the OMPT interface for OpenMP,
                       suppress all samples not in serial code.

  -ds, --delay-sampling
                       Delay starting sampling until the application calls
                       hpctoolkit_sampling_start().

  -f <frac>, -fp <frac>, --process-fraction <frac>
                       Measure only a fraction <frac> of the execution's
                       processes.  For each process, enable measurement
                       (of all threads) with probability <frac>; <frac> is a
                       real number (0.10) or a fraction (1/10) between 0 and 1.

  -m, --merge-threads  Merge non-overlapped threads into one virtual thread.
                       This option is to reduce the number of generated
                       profile and trace files as each thread generates its own
                       profile and trace data. The options are:
                       0 : do not merge non-overlapped threads
                       1 : merge non-overlapped threads (default)

  -o <outpath>, --output <outpath>
                       Directory for output data.
                       {hpctoolkit-<command>-measurements[-<jobid>]}

                       Bug: Without a <jobid> or an output option, multiple
                       profiles of the same <command> will be placed in the
                       same output directory.

  -olr <list>, --only-local-ranks <list>
                       Measure only specified MPI local ranks. <list> is a
                       comma-separated series of local ranks. A local rank
                       is the local index of a rank on a node.
                       {all local ranks measured}

  -r, --retain-recursion
                       Normally, hpcrun will collapse (simple) recursive call chains
                       to save space and analysis time. This option disables that
                       behavior: all elements of a recursive call chain will be recorded
                       NOTE: If the user employs the RETCNT sample source, then this
                             option is enabled: RETCNT implies *all* elements of
                             call chains, including recursive elements, are recorded.

  -nu, --no-unwind     Disable unwinding and collect a flat profile of leaf
                       procedures only instead of full calling contexts.
                       Equivalent to -a flat.

Options to consider only when hpcrun causes an application to fail:

  --disable-auditor    Typically, hpcrun uses LD_AUDIT to track dynamic library
                       operations (see NOTES). This option instructs hpcrun
                       to track dynamic library operations by intercepting
                       dlopen and dlclose instead of using LD_AUDIT. Note
                       that this alternate approach can cause problem with
                       load modules that specify a RUNPATH.

  --enable-auditor     Enable LD_AUDIT support. If the default setting for
                       LD_AUDIT is configured at build time as 'disabled',
                       this flag can be used to enable it.

  --disable-auditor-got-rewriting
                       Typically, hpcrun uses LD_AUDIT to help track dynamic
                       library operations using an 'auditor' library. When using
                       an auditor library, glibc unnecessarily intercepts
                       every call to a function in a shared library. hpcrun
                       avoids this overhead by rewriting each shared library's
                       global offset table (GOT). Such rewriting is tricky.
                       This option can be used to disable GOT rewriting if
                       it is believed that the rewriting is causing the
                       application to fail.

  --namespace-single   dlmopen may load a shared library into an alternate
                       namespace.  Use of dlmopen to create multiple namespaces
                       can cause an application to crash when using glibc < 2.32
                       (see NOTES). This option asks HPCToolkit to load all
                       shared libraries within the application namespace, which
                       might help avoid a crash.

  --namespace-multiple dlmopen may load a shared library into an alternate
                       namespace. Force HPCToolkit to allow dlmopen to create
                       alternate namespaces.

  --disable-gprof      Override and disable gprof instrumentation  This option
                       is only useful when using hpcrun to add HPCToolkit's
                       measurement subsystem to a dynamically-linked application
                       that has been compiled with -pg. One can't measure
                       performance with HPCToolkit when gprof instrumentation
                       is active in the same execution.


NOTES:

* hpcrun uses preloaded shared libraries to initiate profiling.  For this
  reason, it cannot be used to profile setuid programs.

* Typically, hpcrun uses LD_AUDIT to monitor an application's use of dynamic
  libraries. Use of LD_AUDIT is needed to properly interpret a load module's
  RUNPATH attribute. However, use of LD_AUDIT will cause dlmopen to fail and
  can cause heap corruption for stock glibc < 2.32. On systems with stock
  glibc < 2.32, it is recommended that HPCToolkit be configured to disable
  default use of LD_AUDIT to avoid potential heap corruption by glibc. Since
  Intel's GTPin library for instrumentation of Intel GPU binaries uses dlmopen,
  LD_AUDIT is disabled automatically when GTPin is used to avoid LD_AUDIT
  problems on systems with glibc < 2.32.

)==";
}

static std::optional<std::string> fetchenv(const std::string& str) {
  const char* env = std::getenv(str.c_str());
  if (env == NULL || env[0] == '\0')
    return std::nullopt;
  return std::string(env);
}

static bool non_empty(const std::string& arg) {
  return !arg.empty() && arg[0] != '-';
}

static bool strmatch(const std::string& arg, std::initializer_list<const std::string_view> allowed) {
  for (const auto& a: allowed) {
    if (arg == a) return true;
  }
  return false;
}

static bool strstartswith(const std::string_view haystack, const std::string_view needle) {
  return haystack.substr(0, needle.size()) == needle;
}

static char listcmd[] = "/bin/ls";
static char* listargv[] = {listcmd, listcmd, NULL};

int main(int argc, char* argv[]) {
  // This map collects environment modifications we want to pass on to the main application.
  // To keep the running environment clean, we stack these throughout the process and only
  // apply them at the very end.
  std::unordered_map<std::string, std::string> env;

  // On Cray Slingshot 11 systems, there is not yet complete support for demand paging on the
  // Cassini NIC. As a result, when using pinned memory with Cray MPI, fork and system aren't
  // allowed to return because forking a process using pinned memory is unsupported. As a result,
  // libmonitor can't wrap system as it always has. Instead, we simply invoke glibc system
  // directly instead of using libmonitor's reimplementation.
  env["MONITOR_NO_SYSTEM_OVERRIDE"] = "1";

#if !HPCRUN_USE_AUDITOR_BY_DEFAULT
  // LD_AUDIT may be crippled. If it is, use the emulated LD_AUDIT support to avoid
  // consistent failures. This can always be overridden using
  // `--enable-auditor`.
  env["HPCRUN_AUDIT_FAKE_AUDITOR"] = "1";
#endif

  // Recent versions of MPI use UCX as a communication substrate. Prevent UCX from catching SEGV
  // by dropping it from UCX's list of error signals. If UCX catches SEGV, it will terminate the
  // program. We can't allow UCX to do that since hpcrun may encounter a SEGV as part of its
  // operation and want to drop a sample rather than terminate the program.
  env["UCX_ERROR_SIGNALS"] = "ILL,FPE,BUS";

  // Handle "early" version arguments
  if (argc > 1 && strmatch(argv[1], {"-V", "-version", "--version"})) {
    hpctoolkit_print_version_and_features("hpcrun");
    return 0;
  }

  std::deque<fs::path> audit_list;
  std::deque<fs::path> preload_list;
  enum class NsDefault { single, multiple, single_if_auditing } namespace_default = NsDefault::multiple;
  bool namespace_multiple = false;
  bool namespace_single = false;

  env["HPCRUN_DEBUG_FLAGS"] = "";
  env["HPCRUN_EVENT_LIST"] = "";
  env["HPCRUN_CONTROL_KNOBS"] = "";

  // Parse all the arguments, one at a time
  std::string arg0 = argv[0];
  while (argc > 1) {
    std::string arg = argv[1];
    --argc, ++argv;  // shift

    auto popvalue = [&]() -> std::string {
      std::string value = argc > 1 ? argv[1] : std::string();
      if (!non_empty(value)) {
        std::cerr << "hpcrun: missing argument for " << arg << diemsg;
        std::exit(1);
      }
      --argc, ++argv;  // shift
      return value;
    };

    // AMD GPUs need these settings to work with CPU timer interrupts
    // These settings if you even if you don't build with ROCm support
    // but you try to measure GPU-accelerated programs that offload onto
    // AMD GPUs. You might inadvertently fire up ROCm if there is an AMD
    // GPU on your system even if you are trying to run an OpenCL program
    // on another kind of GPU. Set these variables to guard against
    // failure if you happen to use AMD GPUs without gpu=rocm.
    env["HSA_ENABLE_INTERRUPTS"] = "0";
    env["ROCP_HSA_INTERCEPT"] = "1";

    if (strmatch(arg, {"-md", "--monitor-debug"})) {
      env["MONITOR_DEBUG"] = "1";
    } else if (strmatch(arg, {"-d", "--debug"})) {
      env["HPCRUN_WAIT"] = "1";
    } else if (strmatch(arg, {"-dd", "--dynamic-debug"})) {
      env["HPCRUN_DEBUG_FLAGS"] += std::string(" ") + popvalue();
    } else if (strmatch(arg, {"-ad", "--audit-debug"})) {
      env["HPCRUN_AUDIT_DEBUG"] = "1";
    } else if (strmatch(arg, {"-ck", "--control-knob"})) {
      env["HPCRUN_CONTROL_KNOBS"] += std::string(" ") + popvalue();
    } else if (strmatch(arg, {"-nu", "--no-unwind"})) {
      env["HPCRUN_NO_UNWIND"] = "1";
    } else if (strmatch(arg, {"-h", "-help", "--help"})) {
      usage();
      return 0;
    } else if (strmatch(arg, {"-a", "--attribution"})) {
      auto val = popvalue();
      if (strmatch(val, {"flat"})) {
        env["HPCRUN_NO_UNWIND"] = "1";
      } else if (strmatch(val, {"python"})) {
#ifdef ENABLE_LOGICAL_PYTHON
        env["HPCRUN_LOGICAL_PYTHON"] = "1";
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with Python support enabled" << diemsg;
        return 1;
#endif
      } else {
        std::cerr << "hpcrun: Invalid argument for " << arg << ": " << val << diemsg;
        return 1;
      }
    } else if (strmatch(arg, {"-e", "--event"})) {
      auto ev = popvalue();
      if (strstartswith(ev, "GA")) {
        preload_list.emplace_back(HPCRUN_PRELOAD_GA_SO);
      } else if (strstartswith(ev, "IO")) {
        preload_list.emplace_back(HPCRUN_PRELOAD_LIBC_IO_SO);
      } else if (strstartswith(ev, "MEMLEAK")) {
        preload_list.emplace_back(HPCRUN_PRELOAD_LIBC_ALLOC_SO);
      } else if (strstartswith(ev, "PTHREAD_WAIT")) {
        preload_list.emplace_back(HPCRUN_PRELOAD_LIBC_SYNC_SO);
      } else if (strstartswith(ev, "gpu=rocm") || strstartswith(ev, "gpu=amd") || strstartswith(ev, "rocm::")) {
#ifdef USE_ROCM
        if (strstartswith(ev, "gpu=amd")) {
          ev.replace(4,3,"rocm");
          std::cerr <<
           "hpcrun: NOTE: 'gpu=amd' is deprecated and will be removed; "
           "'gpu=rocm' should be used instead\n";
        }
        preload_list.emplace_back(HPCRUN_PRELOAD_ROCM_SO);
        // FIXME:
        //    The following setting is currently necessary for rocprofiler-sdk.
        //    Otherwise, an api callback may occur on a runtime thread where
        //    call stack unwinding when launching GPU operations is useless
        //    (and fatal for HPCToolkit).
        env["AMD_DIRECT_DISPATCH"] = "1";
        env["ROCPROFILER_PC_SAMPLING_BETA_ENABLED"] = "1";
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with AMD ROCm support enabled" << diemsg;
        return 1;
#endif
      } else if (strstartswith(ev, "gpu=cuda") || strstartswith(ev, "gpu=nvidia")) {
#ifdef OPT_HAVE_CUDA
        if (strstartswith(ev, "gpu=nvidia")) {
          ev.replace(4,6,"cuda");
          std::cerr <<
           "hpcrun: NOTE: 'gpu=nvidia' is deprecated and will be removed; "
           "'gpu=cuda' should be used instead\n";
        }
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with NVIDIA CUDA support enabled" << diemsg;
        return 1;
#endif
      } else if (ev == "gpu=opencl") {
#ifdef ENABLE_OPENCL
        preload_list.emplace_back(HPCRUN_PRELOAD_OPENCL_SO);
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with OpenCL support enabled" << diemsg;
        return 1;
#endif
      } else if (ev == "gpu=level0") {
#ifdef USE_LEVEL0
        preload_list.emplace_back(HPCRUN_PRELOAD_LEVEL0_SO);
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with Level0 support enabled" << diemsg;
        return 1;
#endif
      } else if (strstartswith(ev, "gpu=level0,pc")) {
#ifdef USE_LEVEL0
        preload_list.emplace_back("libhpcrun_preload_level0.so");
        env["ZE_ENABLE_TRACING_LAYER"] = "1";
        env["ZET_ENABLE_METRICS"] = "1";
        env["HPCRUN_AUDIT_FAKE_AUDITOR"] = "1";
        namespace_default = NsDefault::single_if_auditing;
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with Level0 support enabled" << diemsg;
        return 1;
#endif
      } else if (strstartswith(ev, "gpu=level0,inst")) {
#ifdef USE_LEVEL0
#ifdef USE_GTPIN
        env["ZET_ENABLE_PROGRAM_INSTRUMENTATION"] = "1";
        env["ZE_ENABLE_TRACING_LAYER"] = "1";
        preload_list.emplace_back(HPCRUN_PRELOAD_LEVEL0_SO);
        env["HPCRUN_AUDIT_FAKE_AUDITOR"] = "1";
        namespace_default = NsDefault::single_if_auditing;
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with GTPin support enabled" << diemsg;
        return 1;
#endif
#else
        std::cerr << "hpcrun: HPCToolkit was not compiled with Level0 support enabled" << diemsg;
        return 1;
#endif
      }
      env["HPCRUN_EVENT_LIST"] += std::string(env["HPCRUN_EVENT_LIST"].empty() ? "" : " ") + ev;
    } else if (strmatch(arg, {"-L", "-l", "--list-events"})) {
      env["HPCRUN_EVENT_LIST"] = "LIST";
    } else if (strmatch(arg, {"-ds", "--delay-sampling"})) {
      env["HPCRUN_DELAY_SAMPLING"] = "1";
    } else if (strmatch(arg, {"-c", "--count"})) {
      env["HPCRUN_PERF_COUNT"] = popvalue();
    } else if (strmatch(arg, {"-t", "--trace"})) {
      if (!env["HPCRUN_TRACE"].empty()) {
        std::cerr << "hpcrun: -t/--trace is incompatible with -tt/--ttrace" << diemsg;
        return 1;
      }
      env["HPCRUN_TRACE"] = "1";
    } else if (strmatch(arg, {"-tt", "--ttrace"})) {
      if (!env["HPCRUN_TRACE"].empty()) {
        std::cerr << "hpcrun: -tt/--ttrace is incompatible with -t/--trace" << diemsg;
        return 1;
      }
      env["HPCRUN_TRACE"] = "2";
    } else if (strmatch(arg, {"-js", "--jobs-symtab"})) {
      // Deprecated
      popvalue();
    } else if (strmatch(arg, {"-o", "--output"})) {
      env["HPCRUN_OUT_PATH"] = popvalue();
    } else if (strmatch(arg, {"-olr", "--only-local-ranks"})) {
      env["HPCRUN_LOCAL_RANKS"] = popvalue();
    } else if (strmatch(arg, {"--disable-gprof"})) {
      preload_list.emplace_back(HPCRUN_PRELOAD_GPROF_SO);
    } else if (strmatch(arg, {"--omp-serial-only"})) {
      env["HPCRUN_OMP_SERIAL_ONLY"] = "1";
    } else if (strmatch(arg, {"--namespace-multiple"})) {
      if (namespace_single) {
        std::cerr << "hpcrun: can't use both --namespace-single and --namespace-multiple" << diemsg;
        return 1;
      }
      namespace_multiple = true;
    } else if (strmatch(arg, {"--namespace-single"})) {
      if (namespace_multiple) {
        std::cerr << "hpcrun: can't use both --namespace-single and --namespace-multiple" << diemsg;
        return 1;
      }
      namespace_single = true;
    } else if (strmatch(arg, {"-r", "--retain-recursion"})) {
      env["HPCRUN_RETAIN_RECURSION"] = "1";
    } else if (strmatch(arg, {"-m", "--merge-threads"})) {
      env["HPCRUN_MERGE_THREADS"] = popvalue();
    } else if (strmatch(arg, {"-lm", "--low-memsize"})) {
      env["HPCRUN_LOW_MEMSIZE"] = popvalue();
    } else if (strmatch(arg, {"-ms", "--memsize"})) {
      env["HPCRUN_MEMSIZE"] = popvalue();
    } else if (strmatch(arg, {"-f", "-fp", "--process-fraction"})) {
      env["HPCRUN_PROCESS_FRACTION"] = popvalue();
    } else if (strmatch(arg, {"-mp", "--memleak-prob"})) {
      env["HPCRUN_MEMLEAK_PROB"] = popvalue();
    } else if (strmatch(arg, {"--disable-auditor-got-rewriting"})) {
      env["HPCRUN_AUDIT_DISABLE_PLT_CALL_OPT"] = "1";
    } else if (strmatch(arg, {"--disable-auditor"})) {
      env["HPCRUN_AUDIT_FAKE_AUDITOR"] = "1";
    } else if (strmatch(arg, {"--enable-auditor"})) {
      env["HPCRUN_AUDIT_FAKE_AUDITOR"] = "";
    } else if (arg == "--") {
      break;
    } else if (strstartswith(arg, "-")) {
      std::cerr << "hpcrun: unknown or invalid option: " << arg << diemsg;
      return 1;
    } else {
      // Undo the last shift, all remaining arguments will be passed to the application
      ++argc, --argv;
      break;
    }
  }

  // Add default sampling source if needed
  if (env["HPCRUN_EVENT_LIST"].empty()) {
    env["HPCRUN_EVENT_LIST"] = "CPUTIME@5000";
  } else if (env["HPCRUN_EVENT_LIST"] == "RETCNT") {
    env["HPCRUN_EVENT_LIST"] = "CPUTIME@5000 RETCNT";
  }

  // There must be a command to run, unless -L is set
  if (argc <= 1) {
    if (env["HPCRUN_EVENT_LIST"] == "LIST") {
      argc = 2;
      argv = listargv;
    } else {
      std::cerr << "hpcrun: no command to profile" << diemsg;
      return 1;
    }
  }

  // collapse namespaces, if necessary
  if (namespace_default == NsDefault::single_if_auditing) {
    namespace_default = env["HPCRUN_AUDIT_FAKE_AUDITOR"] == "1" ? NsDefault::multiple : NsDefault::single;
  }
  if (!namespace_multiple) {
    if (namespace_default == NsDefault::single || namespace_single) {
      preload_list.emplace_front(HPCRUN_PRELOAD_LIBDL_DLMOPEN_SO);
    }
  }

  // FIXME: The original hpcrun launch script checked here that the command to execute was in fact
  // a binary of the correct bit-ness and was dynamically linked. This sanity check is reasonable
  // but also prevents valid cases and is hard to implement. It has been skipped for now.

  // Disable the darshan I/O library.  This intercepts some I/O functions
  // inside signal handlers and can cause deadlock.
  env["DARSHAN_DISABLE"] = "1";

  // FIXME: The original hpcrun launch script here checked if the main executable contained _mp_init
  // and added `-dd OMP_SKIP_MSB` if that were the case. This seems to be a hack to work around
  // weird thread stacks in the PGI OpenMP implementation. Since this check is also hard to
  // implement it has been skipped for now.
  // A better check would dlsym() in libhpcrun.so instead of unportable mess in the wrapper script.

  // Inject the main libhpcrun.so with LD_PRELOAD
  preload_list.emplace_front(HPCRUN_PRELOAD_SO);

  // Set up the auditor or fake auditor, depending on what's configured.
  if (env["HPCRUN_AUDIT_FAKE_AUDITOR"] == "1") {
    preload_list.emplace_front(HPCRUN_PRELOAD_LIBDL_SO);
  } else {
    audit_list.emplace_front(HPCRUN_AUDIT_SO);
  }

  if (!preload_list.empty()) {
    std::ostringstream ss;
    bool first = true;
    for (const auto& s: std::move(preload_list)) {
      ss << (first ? "" : ":") << s.native();
      first = false;
    }
    if (auto oldpreload = fetchenv("LD_PRELOAD"))
      ss << ':' << oldpreload.value();
    env["LD_PRELOAD"] = ss.str();
  }

  if (!audit_list.empty()) {
    std::ostringstream ss;
    bool first = true;
    for (const auto& s: std::move(audit_list)) {
      ss << (first ? "" : ":") << s.native();
      first = false;
    }
    if (auto oldaudit = fetchenv("LD_AUDIT"))
      ss << ':' << oldaudit.value();
    env["LD_AUDIT"] = ss.str();
  }

  // Extend LD_LIBRARY_PATH with the path to the libhpcrun libraries. We append here so that the
  // hardcoded path used here can be overridden by users with ease.
  {
    std::ostringstream ss;
    if (auto oldldpath = fetchenv("LD_LIBRARY_PATH"))
      ss << oldldpath.value() << ':';
    ss << HPCRUN_INSTALL_LIBDIR;
    env["LD_LIBRARY_PATH"] = ss.str();
  }

  // Apply the environment we've been collecting all this time, and exec
  for (const auto& [name, value]: std::move(env)) {
    ::setenv(name.c_str(), value.c_str(), 1);
  }
  ::execvp(argv[1], &argv[1]);
  ::perror("exec failed");
  return 127;
}
