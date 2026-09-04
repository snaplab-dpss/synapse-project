#pragma once

#include <stdint.h>

#include "nf.h"
#include "nf-util.h"

struct nf_config {
  uint32_t num_estimators;
  uint32_t scaling;

  uint32_t log_num_estimators;
  uint32_t hash_mask;
  uint32_t rank_mask;
  uint32_t offset;
  uint32_t magnify_factor;
  uint32_t lc_offset;
  uint32_t lc_threshold;
};
