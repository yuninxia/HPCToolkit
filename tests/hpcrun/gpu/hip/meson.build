# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

if not rocm_dep.found() or not has_hip
  subdir_done()
endif

_tstexe_vecadd_hip = custom_target(
  output: 'tstexe-vecadd-hip',
  command: hip_ld,
  input: hip.process(files('vecadd.hip.cpp')),
)

_preargs = ['--rocprofiler-path', rocprofiler_path]

_tst = find_program(files('tst-amd-vecadd-produces-profiles'))
foreach event : ['gpu=amd', 'gpu=rocm']
  test(
    f'Measurement of tstexe-vecadd-hip produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_hip, event, ''] + _preargs,
    suite: ['hpcrun', 'rocm'],
    depends: hpcrun_test_depends,
  )
  test(
    f'Measurement of tstexe-vecadd-hip with tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_hip, event, '-t'] + _preargs,
    suite: ['hpcrun', 'rocm'],
    depends: hpcrun_test_depends,
  )
  test(
    f'Measurement of tstexe-vecadd-hip with boosted tracing produces profiles (@event@)',
    _tst,
    args: [hpctesttool, hpcrun, _tstexe_vecadd_hip, event, '-tt'] + _preargs,
    suite: ['hpcrun', 'rocm'],
    depends: hpcrun_test_depends,
  )
endforeach
