<!--
SPDX-FileCopyrightText: Contributors to the HPCToolkit Project

SPDX-License-Identifier: CC-BY-4.0
-->

# Monitoring MPI

HPCToolkit's measurement subsystem can measure each process and thread in an execution of an MPI program.
HPCToolkit supports C, C++, Fortran, and Python MPI programs.
HPCToolkit can be used with pure MPI programs as well as hybrid programs that use multithreading, e.g., OpenMP or Pthreads, within MPI processes.

A single installation of HPCToolkit should work with any MPI implementation.
You don't need to provide an `mpi.h` include path when building HPCToolkit and you don't need to compile multiple versions of HPCToolkit, one for each MPI implementation.
It has been tested and used with MPICH, MVAPICH and OpenMPI.

```{hint}
A thoughtful reader might wonder how one installation of HPCToolkit can interoperate with MPI implementations that
use different representations for `MPI_COMM_WORLD`.
Instead of calling `MPI_Comm_rank` directly, `hpcrun` waits for the application to call `MPI_Comm_rank` and captures the returned rank index.
```

## Measuring MPI Ranks

For a dynamically linked application binary `app`, use a command line similar to the following example:

```
<mpi-launcher> hpcrun -e <event>:<period> ... app [app-arguments]
```

Observe that the MPI launcher (e.g., `srun`, `mpiexec`, or `mpirun`) is used to launch `hpcrun`, which then launches the application program.

In this example, `s3d_f90.x` is the Fortran S3D program compiled with OpenMPI's `mpif90` and run with the command line

```
mpiexec -n 4 hpcrun -e cycles ./s3d_f90.x
```

This produced 12 files in the following abbreviated `ls` listing:

```
krentel 1889240 Feb 18  s3d_f90.x-000000-000-72815673-21063.hpcrun
krentel    9848 Feb 18  s3d_f90.x-000000-001-72815673-21063.hpcrun
krentel 1914680 Feb 18  s3d_f90.x-000001-000-72815673-21064.hpcrun
krentel    9848 Feb 18  s3d_f90.x-000001-001-72815673-21064.hpcrun
krentel 1908030 Feb 18  s3d_f90.x-000002-000-72815673-21065.hpcrun
krentel    7974 Feb 18  s3d_f90.x-000002-001-72815673-21065.hpcrun
krentel 1912220 Feb 18  s3d_f90.x-000003-000-72815673-21066.hpcrun
krentel    9848 Feb 18  s3d_f90.x-000003-001-72815673-21066.hpcrun
krentel  147635 Feb 18  s3d_f90.x-72815673-21063.log
krentel  142777 Feb 18  s3d_f90.x-72815673-21064.log
krentel  161266 Feb 18  s3d_f90.x-72815673-21065.log
krentel  143335 Feb 18  s3d_f90.x-72815673-21066.log
```

The measurement files show that the execution consisted of four processes with two threads per process.
Looking at the file names, `s3d_f90.x` is the name of the program binary, `000000-000` through `000003-001` are the MPI rank and thread numbers, and `21063` through `21066` are the process IDs.

We see from the file sizes that OpenMPI is spawning one helper thread per process.
Technically, the smaller `.hpcrun` files imply only a smaller calling-context tree (CCT), not necessarily fewer samples.
But in this case, the helper threads are not doing much work.

For the best results with HPCToolkit, an application's first call to `MPI_Comm_rank` should use communicator `MPI_COMM_WORLD`.
Nearly all MPI programs already do this, so this is rarely a problem.
For example, in C, a program might begin with:

```
int main(int argc, char **argv)
{
  int size, rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  ...
}
```

The call to `MPI_Comm_rank` should be unconditional, that is all processes should make this call.
In the code above, the call to `MPI_Comm_size` is not necessary for `hpcrun`, although most MPI programs normally call both `MPI_Comm_size` and `MPI_Comm_rank`.

If the first call to `MPI_Comm_rank` is not passed `MPI_COMM_WORLD`,
HPCToolkit should be able to measure the application anyway, although it may not properly identify MPI ranks.
