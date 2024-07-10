#ifndef LEVEL0_NOTIFY_H
#define LEVEL0_NOTIFY_H

#include <level_zero/ze_api.h>

#include "level0-device.h"

void
zeroNotifyKernelStart
(
  ZeDeviceDescriptor* desc
);

void
zeroNotifyKernelEnd
(
  ZeDeviceDescriptor* desc
);

#endif // LEVEL0_NOTIFY_H