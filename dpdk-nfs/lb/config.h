#pragma once

#include <inttypes.h>

#include <rte_ether.h>
#include "nf.h"

struct nf_config {
  uint32_t max_backends;

  // The height of the consistent hashing table.
  // Bigger value induces more memory usage, but can achieve finer granularity.
  // Must be a prime number.
  uint32_t cht_height;

  // Management device (for changing backends).
  uint16_t management_dev;
};
