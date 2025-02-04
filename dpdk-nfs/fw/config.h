#pragma once

#include "nf.h"

#include <stdint.h>

struct nf_config {
  // WAN device, i.e. external
  uint16_t wan_device;

  // Expiration time of flows, in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;
};
