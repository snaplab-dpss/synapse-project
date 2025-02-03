#pragma once

#include <stdint.h>
#include <rte_ether.h>

#include "nf.h"

#define CONFIG_FNAME_LEN 512

struct nf_config {
  // Configuration file containing lines with: "<src dev> <dst dev> <is_internal (0/1)> <dst mac>"
  char devices_cfg_fname[CONFIG_FNAME_LEN];

  // Expiration time of flows, in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;
};
