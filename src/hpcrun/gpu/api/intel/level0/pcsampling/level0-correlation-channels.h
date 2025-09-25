// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LEVEL0_CORRELATION_CHANNELS_H
#define LEVEL0_CORRELATION_CHANNELS_H

#include <stdint.h>

#define LEVEL0_CORRELATION_CHANNEL_BASE 1u

static inline uint64_t
level0CorrelationChannelIndex
(
  int32_t device_id
)
{
  return (device_id >= 0)
           ? (LEVEL0_CORRELATION_CHANNEL_BASE + (uint64_t)device_id)
           : (uint64_t)LEVEL0_CORRELATION_CHANNEL_BASE;
}

#endif // LEVEL0_CORRELATION_CHANNELS_H
