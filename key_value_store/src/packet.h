#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>

#include "constants.h"

namespace netcache {

struct netcache_hdr_t {
  uint8_t op;
  uint8_t key[KV_KEY_SIZE];
  uint8_t val[KV_VAL_SIZE];
  uint8_t status;
  uint16_t port;
} __attribute__((packed));

}; // namespace netcache
