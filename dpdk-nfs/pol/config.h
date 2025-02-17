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

  // Policer rate in B/s
  uint64_t rate;

  // Policer burst size in B
  uint64_t burst;

  // Size of the dynamic filtering table
  uint32_t dyn_capacity;
};
