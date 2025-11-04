# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if not cupti_dep.found() or not has_cuda
  subdir_done()
endif

# FIXME: PC sampling currently does not work on CUDA 13+
_pc_sampling_should_fail = cupti_dep.version().version_compare('>=13.0')

_tstexe_vecadd_cuda = executable('tstexe-vecadd-cuda', files('vecadd.cu'))

_tst = find_program(files('tst-cuda-vecadd-produces-profiles'))
foreach event : ['gpu=cuda']
  test(
    f'Measurement of tstexe-vecadd-cuda produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, event],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
  )
  test(
    f'Measurement of tstexe-vecadd-cuda w/ tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, event, '-t'],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
  )
  test(
    f'Measurement of tstexe-vecadd-cuda w/ boosted tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, event, '-tt'],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
  )
  test(
    f'Measurement of tstexe-vecadd-cuda w/ PC sampling produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, f'@event@,pc'],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
    should_fail: _pc_sampling_should_fail,
    is_parallel: false,  # CUPTI does not allow multiple processes to sample the GPU
    priority: 100,  # High priority to optimize for lack of parallelism
  )
  test(
    f'Measurement of tstexe-vecadd-cuda w/ PC + tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, f'@event@,pc', '-t'],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
    should_fail: _pc_sampling_should_fail,
    is_parallel: false,  # CUPTI does not allow multiple processes to sample the GPU
    priority: 100,  # High priority to optimize for lack of parallelism
  )
  test(
    f'Measurement of tstexe-vecadd-cuda w/ PC + boosted tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_cuda, f'@event@,pc', '-tt'],
    suite: ['hpcrun', 'cuda'],
    depends: hpcrun_test_depends,
    should_fail: _pc_sampling_should_fail,
    is_parallel: false,  # CUPTI does not allow multiple processes to sample the GPU
    priority: 100,  # High priority to optimize for lack of parallelism
  )
endforeach
