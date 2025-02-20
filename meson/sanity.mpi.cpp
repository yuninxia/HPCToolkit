// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#include <mpi.h>

int main() { // NOLINT(modernize-use-trailing-return-type)
  MPI_Init(nullptr, nullptr);
  int x = 42;
  MPI_Bcast(&x, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
