#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../include/sycon/time.h"

namespace sycon {

time_ns_t get_time() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1'000'000'000ul + tp.tv_nsec;
}

} // namespace sycon