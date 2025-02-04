#pragma once

#include <stdint.h>

#include "nf.h"

struct nf_config {
  // WAN device, i.e. external
  uint16_t wan_device;

  // External IP address
  uint32_t external_addr;

  // Expiration time of flows in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;
};
