# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

test(
  'ensure hpcrun does not see getenv in application'.format(simple_tstexe.name()),
  args: [hpctesttool, hpcrun, test-getenv],
  suite: 'hpcrun',
  depends: hpcrun_test_depends,
)
