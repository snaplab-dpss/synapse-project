#pragma once

#include <stdint.h>

namespace sycon {

struct nf_config_t {
  uint16_t in_dev_port;
  uint16_t out_dev_port;
};

extern struct nf_config_t nf_config;

}  // namespace sycon