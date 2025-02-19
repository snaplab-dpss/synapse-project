#pragma once

#include <stdint.h>

#include <rte_ether.h>

struct nf_config {
  struct {
    uint16_t *src_dev;
    uint16_t *dst_dev;
    size_t n;
  } fwd_rules;
};
