#pragma once

#include <stdint.h>

#include <rte_ether.h>

#include "nf.h"

struct nf_config {
  // Size of the kv store
  uint32_t max_entries;
};
