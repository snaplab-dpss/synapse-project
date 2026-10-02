#pragma once

#include <stddef.h>
#include <stdint.h>

#include "nf.h"

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

  // Stages of the /24 HashPipe (at most CRC32_BANK_SIZE, one polynomial each)
  uint32_t stages;

  // Slots per stage of the /24 HashPipe (a power of two)
  uint32_t width;

  // Counting interval in microseconds: every table starts afresh after it
  uint64_t interval;
};
