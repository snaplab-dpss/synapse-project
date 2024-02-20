#include "../include/sycon/time.h"

namespace sycon {

time_ns_t get_time() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000000000ul + tp.tv_nsec;
}

}  // namespace sycon