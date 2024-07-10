#include "level0-notify.h"

void
zeroNotifyKernelStart
(
  ZeDeviceDescriptor* desc
) 
{
  pthread_mutex_lock(&desc->kernel_mutex_);
  desc->kernel_running_ = true;
  pthread_cond_signal(&desc->kernel_cond_);
  pthread_mutex_unlock(&desc->kernel_mutex_);
}

void
zeroNotifyKernelEnd
(
  ZeDeviceDescriptor* desc
) 
{
  pthread_mutex_lock(&desc->kernel_mutex_);
  desc->kernel_running_ = false;
  pthread_cond_signal(&desc->kernel_cond_);
  pthread_mutex_unlock(&desc->kernel_mutex_);
}
