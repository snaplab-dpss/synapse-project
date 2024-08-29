#pragma once

#include <stdint.h>

#include <rte_ether.h>

#include "nf.h"

struct nf_config {
  uint16_t internal_device;
  uint16_t external_device;
  uint32_t max_entries;
};
