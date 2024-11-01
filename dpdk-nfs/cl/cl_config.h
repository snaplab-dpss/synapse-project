#pragma once

#include <stdint.h>

#include "nf.h"

struct nf_config {
  // LAN (i.e. internal) device
  uint16_t lan_device;

  // WAN device, i.e. external
  uint16_t wan_device;

  // Maximum number of flows
  uint32_t max_flows;

  // Maximum allowed number of clients
  uint16_t max_clients;

  // Expiration time in microseconds
  uint64_t expiration_time;

  // Sketch height (i.e. number of rows)
  uint32_t sketch_height;

  // Sketch width (i.e. number of columns)
  uint32_t sketch_width;

  // Sketch cleanup interval in microseconds
  uint64_t sketch_cleanup_interval;
};
