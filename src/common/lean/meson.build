# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

common_lean_srcs = files(
  'BalancedTree.c',
  'OSUtil.c',
  'bichannel.c',
  'binarytree.c',
  'bistack.c',
  'compress_lzma.c',
  'cpuset_hwthreads.c',
  'crypto-hash.c',
  'demangle.c',
  'elf-extract.c',
  'elf-hash.c',
  'elf-helper.c',
  'formats/cctdb.c',
  'formats/metadb.c',
  'formats/primitive.c',
  'formats/profiledb.c',
  'formats/tracedb.c',
  'generic_pair.c',
  'hpcfmt.c',
  'hpcio-buffer.c',
  'hpcio.c',
  'hpcrun-fmt.c',
  'id-tuple.c',
  'mcs-lock.c',
  'pfq-rwlock.c',
  'placeholders.c',
  'procmaps.c',
  'producer_wfq.c',
  'queues.c',
  'spinlock.c',
  'splay-uint64.c',
  'stacks.c',
  'timer.c',
  'usec_time.c',
  'vdso.c',
)
common_lean_deps = [
  libelf_dep,
  libiberty_dep,
  lzma_dep,
  threads_dep,
  subproject('md5-g72cf2cd').get_variable('md5_dep'),
]

test_srcs = files(
  'compress_lzma_test.cpp',
  'crypto-hash-test.cpp',
  'elf-hash-test.cpp',
)

test(
  'common-lean',
  executable(
    'common-lean-test',
    test_srcs,
    common_lean_srcs,
    dependencies: [threads_dep, gmock_dep, common_lean_deps],
  ),
  args: [files('elf-hash-example')],
  protocol: 'gtest',
)
