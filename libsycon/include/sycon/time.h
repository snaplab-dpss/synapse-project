#pragma once

#include <stdint.h>
#include <time.h>

#include "util.h"

namespace sycon {

typedef u64 time_ns_t;
typedef u64 time_ms_t;

time_ns_t get_time();

}  // namespace sycon