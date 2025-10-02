# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

test(
  'Listing metrics does not error',
  hpcrun,
  args: ['-L'],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
)

_env = environment()
_env.prepend('PATH', fs.parent(hpcrun.full_path()))
test(
  'Listing metrics as "hpcrun" does not error',
  find_program('env'),
  args: ['hpcrun', '-L'],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
  env: _env,
)

subdir('cpu')
subdir('threads')
subdir('python')
subdir('gpu/opencl')
subdir('gpu/cuda')
subdir('gpu/hip')
subdir('gpu/sycl')
subdir('other')
