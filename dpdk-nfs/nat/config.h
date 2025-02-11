#pragma once

#include <stdint.h>

#include "lib/state/lpm-dir-24-8.h"
#include "nf.h"
#include "nf-util.h"

struct nf_config {
  // LPM configuration file for routing between the server and the clients.
  // The configuration file expects the following format per line: "<ipv4 addr>/<subnet size> <device>".
  // E.g.:
  // 10.0.0.0/8 0
  // 11.0.0.0/8 1
  char lpm_cfg_file[LPM_CONFIG_FNAME_LEN];

  // WAN devices
  struct {
    uint16_t *devices;
    size_t n;
    uint32_t *external_addrs;
  } wan_devs;

  // Expiration time of flows in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;
};
