// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
  struct perf_event_attr pe;

  // Initialize perf_event_attr with a simple software event
  memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_SOFTWARE;
  pe.config = PERF_COUNT_SW_CPU_CLOCK;
  pe.size = sizeof(pe);
  pe.disabled = 1;
  pe.exclude_kernel = 1; // Don't require kernel access
  pe.exclude_hv = 1;     // Don't require hypervisor access

  // Try to open a simple perf event for the current process
  // Use pid=0 (current process), cpu=-1 (any CPU), no group, no flags
  int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);

  if (fd >= 0) {
    close(fd);
    return 0;
  }
  return 1;
}
