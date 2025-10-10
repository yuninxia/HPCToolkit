// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#if !defined(INSTANCE)
#error INSTANCE not defined
#endif

#if !defined(LIBTYPE)
#error LIBTYPE not defined
#endif

#define CAT2(T, S, I) T##_##S##_##I
#define CAT(T, S, I) CAT2(T, S, I)
#define NAME(S) CAT(LIBTYPE, S, INSTANCE)

__thread int NAME(tlsvar);

int NAME(set)(int val) {
  NAME(tlsvar) = val;
  return NAME(tlsvar);
}
