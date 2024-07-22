#include "level0-sync.h"

void
zeroPCSampleSync
(
  ZeDeviceDescriptor* desc
)
{
  // Lock until data is processed
  pthread_mutex_lock(&desc->data_mutex_);
  while (!desc->data_processed_) {
    pthread_cond_wait(&desc->data_cond_, &desc->data_mutex_);
  }
  pthread_mutex_unlock(&desc->data_mutex_);
  // Reset the flag
  pthread_mutex_lock(&desc->data_mutex_);
  desc->data_processed_ = false;
  pthread_mutex_unlock(&desc->data_mutex_);
}

void
zeroNotifyDataProcessingComplete
(
  ZeDeviceDescriptor* desc
)
{
  pthread_mutex_lock(&desc->data_mutex_);
  desc->data_processed_ = true;
  pthread_cond_signal(&desc->data_cond_);
  pthread_mutex_unlock(&desc->data_mutex_);
}
