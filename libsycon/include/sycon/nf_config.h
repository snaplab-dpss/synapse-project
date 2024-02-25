#pragma once

#include <stdint.h>

namespace sycon {

struct nf_config_t {
  u16 in_dev_port;
  u16 out_dev_port;
};

extern struct nf_config_t nf_config;

}  // namespace sycon