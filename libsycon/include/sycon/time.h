#pragma once

#include <stdint.h>
#include <time.h>

namespace sycon {

typedef uint64_t time_ns_t;
typedef uint64_t time_ms_t;

time_ns_t get_time();

}  // namespace sycon