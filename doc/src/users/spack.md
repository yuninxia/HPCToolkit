<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

# Installing HPCToolkit with Spack

This chapter outlines how to build and install HPCToolkit and Hpcviewer with [Spack](https://spack.readthedocs.io/en/latest/). If you are unfamiliar with Spack, see the section below [Running Spack for the First Time](#spack-first-time).

HPCToolkit proper (`hpcrun`, `hpcstruct` and `hpcprof`) is used to measure and
analyze an application's performance and then produce a database for `hpcviewer`.
We distribute HPCToolkit from source on GitLab and support building on 64-bit
Linux on x86_64, little-endian powerpc (power8, 9 and 10) and ARM (aarch64)
with spack.

We provide binary distributions for `hpcviewer` on Linux (x86_64, ppc64/le
and aarch64), Windows (x86_64) and MacOS (x86_64, M1, M2 and M3).
Although `hpcviewer` is normally included as part of HPCToolkit, we provide
`hpcviewer` as a separate package to install on platforms, such as MacOS and Windows laptops,
where HPCToolkit proper is not available.
HPCToolkit databases are platform-independent and it is common to run
`hpcrun` on one machine and then view the results on another machine.

Building `hpctoolkit` requires a fairly recent C/C++ compiler that supports
C++17 and common GNU extensions (including LLVM).
GNU gcc/g++ 12.x or later should do fine.
Spack requires Python 3.8 or later plus various build utilities:
bash, git, curl, patch, tar, bzip2, unzip, etc.

`Hpcviewer` requires Java 17 or later.
Without any configuration, Spack will automatically install an appropriate version of Java OpenJDK to support `hpcviewer`.

When installing `hpctoolkit` using Spack, `hpcviewer` will be installed automatically by default.

Gitlab repositories:

- <https://gitlab.com/hpctoolkit/hpctoolkit>
- <https://gitlab.com/hpctoolkit/hpcviewer>

Spack Documentation:

- <https://spack.readthedocs.io/en/latest>

## Config Files

Spack uses a variety of config files for setting paths, options, etc.
The simplest way to set or change a value is to copy the default file from
`spack/etc/spack/defaults` one directory up and make the change there.

A complete discussion of this topic is available in Spack's documentation about [configuration files](https://spack.readthedocs.io/en/latest/configuration.html).
Here, we only mention a few key configuration details that you might want to adjust when installing HPCToolkit with Spack.

### Config.yaml

In `config.yaml`, you may want to set the path to the install tree.

```
config:
  # This is the path to the root of the Spack install tree.
  install_tree:
    root:  /path/to/root/of/install/tree
```

### Modules.yaml

If you are using modules (TCL or Lmod), you need to `enable` the module type
and provide the path to the modules directory.

```
modules:
  ...
  default:
    # Normally only need one of these
    roots:
      tcl:   /path/to/tcl/modules/directory
      lmod:  /path/to/lmod/modules/directory
    # What type of modules to use ("tcl" and/or "lmod")
    enable:
     - tcl
     - lmod
```

```{important}
You should disable the module `autoload` feature for `hpctoolkit`.
HPCToolkit does not need its dependency modules loaded and loading them
may interfere with your application's dependencies.
Do this for both `tcl` and `lmod` modules (whichever one you are using).
```

```
modules:
  ...
  default:
    ...
    tcl:
      hpctoolkit:
        autoload: none
      all:
        autoload: direct
    lmod:
      hpctoolkit:
        autoload: none
      all:
        autoload: direct
```

```{tip}
If you forget to turn off `autoload` for `hpctoolkit`, you don't have
to start over from scratch.
You can uninstall `hpctoolkit`, fix the `modules.yaml` file and then
reinstall `hpctoolkit`.
Or, you could just hand edit the module file.
```

## Installing a Basic HPCToolkit

Here we explain how to install a basic version of HPCToolkit with Spack.
The directions immediately below build and install a version of HPCToolkit
that profiles only a program's CPU activity.
To install a GPU-aware version of HPCToolkit or configure HPCToolkit for post-mortem
analysis of thousands of profiles, see the [Configuration Options](#configuration-options)
section on this page for a description of how to enable optional HPCToolkit features when building
with Spack.

Although spack can install packages with a single `spack install`
command, we recommend that you always run `spack spec` first to verify
what spack is going to install.

```
spack spec hpctoolkit
```

Check that the version and variants for `hpctoolkit` and `dyninst` and
the compiler and `arch` type are what you want.
If you are adding support for a GPU or `hpcprof-mpi`, check that they
are included and are the version that you intend.
Then, install HPCToolkit with:

```
spack install hpctoolkit
```

```{important}
Normally, Spack builds HPCToolkit for the specific architecture (skylake, sapphirerapids, zen3, etc) on which `spack install hpctoolkit` is run.
Some care is required when installing HPCToolkit on a system that contains multiple kinds of x86_64 processors. A default Spack build of HPCToolkit on one kind of x86_64 processor and running on another may cause `illegal instruction` errors.

It is worth noting that in some cases, processor heterogeneity is less than obvious. For example, on Argonne's Aurora supercomputer, login nodes use Intel Icelake processors while compute nodes use Intel Sapphire Rapids processors.

You can avoid problems when multiple kinds of x86_64 processors are present by configuring HPCToolkit for a generic x86_64 processor as follows:

`spack install hpctoolkit target=x86_64`
```

HPCToolkit can also be configured for a generic x86_64 target in `packages.yaml` with:

```
packages:
  all:
    require: "target=x86_64"
```

You can see a list Spack's targets and families with `spack arch`.

```
spack arch --known-targets
```

For further information, see Spack's documentation about [architecture specifiers](https://spack.readthedocs.io/en/latest/spec_syntax.html#architecture-specifiers).

```{tip} Configuring HPCToolkit for debugging
By default, Spack will build a `release` version of `hpctoolkit`
(full optimization and no debug info).
If you want a `debug` version, use the `buildtype` variant.

`
spack install hpctoolkit buildtype=debug
`

Possible values for `buildtype` are `release` (default), `debug`,
`debugoptimized`, `minsize` and `plain`.
```

```{tip}
If you encounter problems with the latest release of HPCToolkit, you might trying the in-development versions of HPCToolkit and Dyninst; namely, `hpctoolkit@develop` and `dyninst@master`:

   `spack install hpctoolkit@develop ^dyninst@master`
```

## Installing Hpcviewer

HPCToolkit's `hpcviewer` user interface is installed by default when installing `hpctoolkit` with Spack.
However, `hpcviewer` is also available as a separate package.
If you want to run `hpcviewer` on a platform (eg, laptop) without installing
all of `hpctoolkit`, then you can install just the `hpcviewer` package.

```
spack install hpcviewer
```

We provide binary distributions for HPCToolkit's `hpcviewer` graphical user interface
on Linux (x86_64, ppc64/le
and aarch64), Windows (x86_64) and MacOS (x86_64 and Apple M-series processors).
`Hpcviewer` uses Java 17 or later, but this is installed automatically by Spack as a dependency
of `hpcviewer`.

```{note}
HPCToolkit databases are platform-independent and it is common to run
`hpcrun` on one machine and then view the results on another machine.
```

(configuration-options)=

## Configuration Options

### CUDA (`+cuda`)

HPCToolkit supports profiling nVidia GPUs through the CUDA interface.
You can either use an existing CUDA module or let Spack build one.

To use an existing CUDA module (recommended), load the `cuda` module
and run `spack external find`.
For example:

```
module load cuda/12.9
spack external find cuda
```

This should create an entry in `packages.yaml` similar to the following.
If not, then add an entry manually.

```
packages:
  cuda:
    externals:
    - spec: cuda@12.9
      prefix: /usr/local/cuda-12.9
```

Then, build `hpctoolkit` with `+cuda`.

```
spack install hpctoolkit +cuda
```

### Level Zero (`+level_zero`)

Beginning with 2025.0, HPCToolkit has basic support for Intel GPUs
(start and stop times for GPU kernels) through the Intel Level Zero interface.
Build `hpctoolkit` with `+level_zero`.

```
spack install hpctoolkit +level_zero
```

Advanced support to see inside the GPU kernels requires the `oneapi-igc`
(Intel Graphics Compiler) package.
This is an externals only package, Spack does not download or install it.
Normally, this is installed in `/usr` and you should manually add a
Spack externals entry for it.

```
packages:
  oneapi-igc:
    externals:
    - spec: oneapi-igc@1.0.10409
      prefix: /usr
```

Then build `hpctoolkit` with both `+level_zero` and `+gtpin`.

```
spack install hpctoolkit +level_zero +gtpin
```

We recommend always building with gtpin if possible and then deciding
at runtime which options to use.

### ROCm (`+rocm`)

HPCToolkit supports profiling AMD GPUs through the ROCm interface.
You can either use an existing ROCm module or let Spack build one.

For ROCm support, HPCToolkit uses four Spack packages:
`hip`, `hsa-rocr-dev`, `roctracer-dev` and `rocprofiler-dev`.
To use an existing ROCm module (recommended), load the `rocm` module
and run `spack external find` on all four packages.
For example:

```
module load rocm/6.4.0
spack external find hip hsa-rocr-dev roctracer-dev rocprofiler-dev
```

This should create entries in `packages.yaml` similar to the following.
If not, then add them manually.

```
packages:
  hip:
    externals:
    - spec: hip@6.4.0
      prefix: /opt/rocm-6.4.0
  roctracer-dev:
    externals:
    - spec: roctracer-dev@6.4.0
      prefix: /opt/rocm-6.4.0
  hsa-rocr-dev:
    externals:
    - spec: hsa-rocr-dev@6.4.0
      prefix: /opt/rocm-6.4.0
  rocprofiler-dev:
    externals:
    - spec: rocprofiler-dev@6.4.0
      prefix: /opt/rocm-6.4.0
```

Then, build `hpctoolkit` with `+rocm`.

```
spack install hpctoolkit +rocm
```

### OpenCL (`+opencl`)

For all three GPU types, an application can access the GPU through the
native interface (CUDA, ROCm, Level Zero) or through the OpenCL interface.
To add support for OpenCL, use the `+opencl` variant in addition to
the native interface.

For example, with CUDA:

```
spack install hpctoolkit +cuda +opencl
```

We recommend adding opencl support for all GPU types.

### MPI (`+mpi`)

HPCToolkit always supports profiling MPI applications.
The Spack variant `+mpi` is for building `hpcprof-mpi`, the MPI version of `hpcprof`.
If you want to build `hpcprof-mpi`, then you need to supply an installation of MPI.

```
spack install hpctoolkit +mpi
```

Normally, for systems with compute nodes, you should use an existing MPI module that
was built for the correct interconnect for your system and add this to `packages.yaml`.
The MPI module should be built with the same C/C++ compiler used to
build `hpctoolkit` (to keep the C++ libraries in sync).
For example:

```
packages:
  mpi:
    require: "mpich@4.1.2"
  mpich:
    externals:
    - spec: mpich@4.1.2
      modules:
      - mpich/4.1.2
```

```{note}
This version of MPI is for `hpcprof-mpi` only and is entirely separate from
any MPI in your application.
```

### PAPI vs Perfmon (`+papi`)

HPCToolkit can access the Hardware Performance Counters with either
PAPI (default) or Perfmon (`libpfm4`).
PAPI runs on top of the perfmon library and uses its own, internal
(but slightly out of date) copy of perfmon.
So, building with `+papi` allows accessing the counters with either
PAPI or perfmon events.

If you want to disable PAPI and use the latest Perfmon instead, then
build hpctoolkit with `~papi`.

```
spack install hpctoolkit ~papi
```

### Python (`+python`)

HPCToolkit supports profiling Python scripts and attributing samples
to Python source functions and not the Python interpreter.
Use the `+python` variant.

```
spack install hpctoolkit +python
```

You should use python 3.10 or later and use the same version as the application.

(spack-first-time)=

## Running Spack for the First Time

If you have never run Spack before, then follow these steps to set up your system.
Some of these steps may take several minutes for the first time while Spack
initializes its files.

Start by cloning Spack from GitHub.

```
git clone https://github.com/spack/spack.git
```

Source the Spack setup script for your shell.
This adds `spack` to your `PATH` and sets up support for `spack load`.
For example, for `bash:`

```
cd spack/share/spack
. setup-env.sh
```

If `/usr/bin/python3` is older than 3.8 or 3.9, then find or install a
later version and set `SPACK_PYTHON` to that path.
This is the version of `python3` used to run the Spack scripts.
If a package uses python as a dependency, then that is separate.
For example:

```
export SPACK_PYTHON=/usr/bin/python3.11
```

Run `spack info` on a small package.
This will trigger cloning the Spack packages repository.

```
spack info zlib
```

Run `spack compiler find` to search for available compilers on your system.

```
spack compiler find
```

Run `spack solve` or `spack spec` on a small package.
This will cause Spack to install the concretizer, Spack's main engine for
resolving specs.

```
spack solve zlib
```

Then you're ready to start installing packages.
Edit `config.yaml` and `modules.yaml` as described above, run
`spack external find` as needed, and then install `hpctoolkit`.

If your system compiler is too old, you can have Spack build a later one.
For example, on one RHEL 8.x system, `/usr/bin/gcc` is 8.5.0.
I used that to build gcc 14.3.0 and then used gcc 14.3.0 to build `hpctoolkit`.

```
spack install gcc@14.3.0 %gcc@8.5.0
spack load gcc@14.3.0    (or module load)
spack compiler find
spack install hpctoolkit %gcc@14.3.0
```

For more information about using Spack, consult Spack's [Getting Started](https://spack.readthedocs.io/en/latest/getting_started.html) documentation.
