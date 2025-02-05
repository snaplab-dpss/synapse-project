#pragma once

#include <stdint.h>

#include "lib/state/lpm-dir-24-8.h"
#include "nf.h"

struct nf_config {
  // LPM configuration file for routing between the server and the clients.
  // The configuration file expects the following format per line: "<ipv4 addr>/<subnet size> <device>".
  // E.g.:
  // 10.0.0.0/8 0
  // 11.0.0.0/8 1
  char lpm_cfg_file[LPM_CONFIG_FNAME_LEN];

  // Internal server
  uint16_t server_dev;

  // Cache capacity
  uint32_t capacity;

  // Expiration time (microseconds)
  uint32_t expiration_time_us;
};
