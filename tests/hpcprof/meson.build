# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

_tst = find_program(files('tst-flags-effective'))
foreach name, meas : testdata_meas
  test(
    f'Flags on @name@ are effective',
    _tst,
    args: [hpctesttool, hpcprof, meas['dir']],
    suite: 'hpcprof',
    timeout: 60,  # These tests run hpcprof twice
  )
endforeach

_tst = find_program(files('tst-accuracy'))
foreach name, dbase : testdata_dbase
  foreach threads : [1, 3]
    test(
      f'Database from @name@ is accurate (-j@threads@)',
      _tst,
      args: [
        hpctesttool,
        dbase['dir'],
        hpcprof,
        f'-j@threads@',
        dbase['args'],
        dbase['measurements']['dir'],
      ],
      suite: 'hpcprof',
      timeout: 30 * threads,  # Multithreaded tests may need extra time
    )
  endforeach

  if mpi_dep.found() and mpiexec.found()
    foreach x : [[1, 1], [3, 1], [2, 2]]
      ranks = x[0]
      threads = x[1]
      test(
        f'Database from @name@ is accurate (ranks=@ranks@ -j@threads@)',
        _tst,
        args: [
          hpctesttool,
          dbase['dir'],
          mpiexec_cmd,
          f'@ranks@',
          hpcprof_mpi,
          f'-j@threads@',
          dbase['args'],
          dbase['measurements']['dir'],
        ],
        suite: ['hpcprof', 'mpi'],
        is_parallel: false,  # Needed to deal with touchy MPI implementations
        priority: -100,  # Normal, threaded tests should run first
        timeout: 0,
      )
    endforeach
  endif
endforeach
