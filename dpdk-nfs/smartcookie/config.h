#pragma once

#include <stdint.h>

#include "nf.h"
#include "nf-util.h"

#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

// UDP destination port of the server agent's time-sync packets.
#define TIMESYNC_PORT 5555

struct nf_config {
  // The one device facing the protected server; every other device faces clients.
  uint16_t server_dev;

  // HalfSipHash key.
  uint32_t sip_key_0;
  uint32_t sip_key_1;

  // Bloom Filter height (i.e. number of rows) and width (i.e. number of columns).
  uint32_t bloom_filter_height;
  uint32_t bloom_filter_width;

  // HalfSipHash initial state (key xor constants), derived from the key at init.
  uint32_t sip_init[4];
};
