#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <time.h>

#include "util.h"

namespace sycon {

typedef u64 time_ns_t;
typedef u64 time_ms_t;
typedef u64 time_us_t;
typedef u64 time_s_t;

inline time_ns_t get_time() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1'000'000'000ul + tp.tv_nsec;
}

} // namespace sycon