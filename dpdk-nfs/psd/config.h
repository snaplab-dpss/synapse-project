#pragma once

#include <stdint.h>

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

  // Maximum number of concurrent sources
  uint32_t capacity;

  // Maximum allowed number of touched ports
  uint32_t max_ports;

  // Expiration time of sources in microseconds
  uint32_t expiration_time;
};
