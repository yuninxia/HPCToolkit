<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

# HPCToolkit

HPCToolkit is an integrated suite of tools for measurement and
analysis of program performance on computers ranging from multicore
desktop systems to GPU-accelerated supercomputers. HPCToolkit
provides accurate measurements of a program's work, resource
consumption, and inefficiency, correlates these metrics with the
program's source code, works with multilingual, fully optimized
binaries, has very low measurement overhead, and scales to large
parallel systems. HPCToolkit's measurements provide support for
analyzing a program execution cost, inefficiency, and scaling
characteristics both within and across nodes of a parallel system.

## Supported platforms

HPCToolkit is supported on GNU/Linux for the following architectures:

|   Architecture | GCC           | DPKG (Debian/Ubuntu) | RPM (Fedora/RHEL/SUSE) |
| -------------: | ------------- | -------------------- | ---------------------- |
|   Intel x86-64 | `x86_64`      | `amd64`              | `x86_64`               |
|     ARM 64-bit | `aarch64`     | `arm64`              | `aarch64`              |
| IBM Power (LE) | `powerpc64le` | `ppc64el`            | `ppc64le`              |

The sibling HPCViewer graphical explorer supports a wider range of platforms, see [the HPCViewer repository](https://gitlab.com/hpctoolkit/hpcviewer) for details.

## Getting HPCToolkit via Spack (recommended for users)

For system administrators or users, the recommended way to install HPCToolkit using [Spack](https://spack.readthedocs.io/en/latest). If you have an installation of Spack at your fingertips, you can inquire about HPCToolkit installation options as shown below:

```console
$ spack info hpctoolkit
```
You can install HPCToolkit with Spack as shown below:

```console
$ spack install hpctoolkit [+features...]
```

You can load a version of HPCToolkit installed with Spack as shown below:

```console
$ spack load hpctoolkit [+features...]
```

For more information about Spack and how to install HPCToolkit using Spack, see the HPCToolkit User's Manual section [Installing HPCToolkit using Spack](https://hpctoolkit.gitlab.io/hpctoolkit/users/spack.html)


## Building HPCToolkit from source with Meson (recommended for developers)

HPCToolkit supports building from source using [Meson].

For information about Meson and how to build HPCToolkit from source using Meson, see the HPCToolkit User's Manual section [Building from Source using Meson](https://hpctoolkit.gitlab.io/hpctoolkit/users/meson.html)

## Documentation

The HPCToolkit User Manual is available online.

## License

This source distribution as a whole is licensed under the [`LICENSE`](./LICENSE). This source distribution follows [REUSE Specification] Version 3 to declare copyright and licensing at file granularity, the individual license texts are provided in the `LICENSES/` subdirectory.

[reuse specification]: https://reuse.software/spec/
