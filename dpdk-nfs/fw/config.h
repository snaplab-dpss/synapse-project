#pragma once

#include <stdint.h>

#include "lib/state/lpm-dir-24-8.h"
#include "nf.h"
#include "nf-util.h"

struct nf_config {
  struct {
    uint16_t *devices;
    size_t n;
  } internal_devs;

  struct {
    uint16_t *src_dev;
    uint16_t *dst_dev;
    size_t n;
  } fwd_rules;

  // Expiration time of flows, in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;
};
