#pragma once

#include <stdint.h>

#include "nf.h"

struct nf_config {
  // External device
  uint16_t client_dev;

  // Internal device
  uint16_t server_dev;

  // Cache capacity
  uint32_t capacity;

  // Expiration time (us)
  uint32_t expiration_time_us;
};
