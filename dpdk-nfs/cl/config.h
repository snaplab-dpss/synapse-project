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
