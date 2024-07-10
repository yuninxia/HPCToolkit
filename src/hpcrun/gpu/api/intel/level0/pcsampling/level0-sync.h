#ifndef LEVEL0_SYNC_H
#define LEVEL0_SYNC_H

#include <pthread.h>

#include "../level0-data-node.h"
#include "level0-device.h"

void
zeroPCSampleSync
(
  ZeDeviceDescriptor* desc
);

void
zeroNotifyDataProcessingComplete
(
  ZeDeviceDescriptor* desc
);

#endif  // LEVEL0_SYNC_H