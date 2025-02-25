#pragma once

#include <stdint.h>

#include "nf.h"
#include "nf-util.h"

struct nf_config {
  // Internal server
  uint16_t server_dev;

  // Cache capacity
  uint32_t capacity;

  // Expiration time (microseconds)
  uint32_t expiration_time_us;
};
