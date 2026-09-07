#pragma once

#include <stdint.h>

#include "lib/util/math.h"

// HalfSipHash-2-4 over four 32-bit words, as computed by the SmartCookie switch agent
// (tofino/smartcookie/p4/smartcookie.p4). The agent omits the standard finalization
// constant (v[2] ^= 0xff), so this does too: cookies must match the switch, not the RFC.

#define HALFSIPHASH_CONST_0 0x70736575
#define HALFSIPHASH_CONST_1 0x6e646f6d
#define HALFSIPHASH_CONST_2 0x6e657261
#define HALFSIPHASH_CONST_3 0x79746573

static inline void halfsiphash_init(uint32_t v[4], uint32_t key_0, uint32_t key_1) {
  v[0] = key_0 ^ HALFSIPHASH_CONST_0;
  v[1] = key_1 ^ HALFSIPHASH_CONST_1;
  v[2] = key_0 ^ HALFSIPHASH_CONST_2;
  v[3] = key_1 ^ HALFSIPHASH_CONST_3;
}

static inline void sipround(uint32_t v[4]) {
  v[0] += v[1];
  v[2] += v[3];
  v[1] = rotate_left(v[1], 5);
  v[3] = rotate_left(v[3], 8);
  v[1] ^= v[0];
  v[3] ^= v[2];
  v[0] = rotate_left(v[0], 16);
  v[2] += v[1];
  v[0] += v[3];
  v[1] = rotate_left(v[1], 13);
  v[3] = rotate_left(v[3], 7);
  v[1] ^= v[2];
  v[3] ^= v[0];
  v[2] = rotate_left(v[2], 16);
}

static inline uint32_t halfsiphash(const uint32_t init[4], uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) {
  uint32_t v[4] = {init[0], init[1], init[2], init[3]};
  uint32_t m[4] = {w0, w1, w2, w3};

  for (int i = 0; i < 4; i++) {
    v[3] ^= m[i];
    sipround(v);
    sipround(v);
    v[0] ^= m[i];
  }

  for (int i = 0; i < 4; i++) {
    sipround(v);
  }

  return v[0] ^ v[1] ^ v[2] ^ v[3];
}
