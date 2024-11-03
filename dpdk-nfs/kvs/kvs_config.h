#pragma once

#include <stdint.h>

#include <rte_ether.h>

#include "nf.h"

struct nf_config {
  uint16_t internal_device;
  uint16_t external_device;

  // Cache capacity
  uint32_t capacity;

  // Number of samples to probe in the cache when updating it
  uint32_t sample_size;

  // Sketch height (i.e. number of rows)
  uint32_t sketch_height;

  // Sketch width (i.e. number of columns)
  uint32_t sketch_width;

  // Sketch cleanup interval in microseconds
  uint64_t sketch_cleanup_interval;

  // Heavy hitter threshold
  uint64_t hh_threshold;
};
