#include "math.h"

#include <stdint.h>

unsigned hash_obj(void *obj, unsigned size_bytes) {
  unsigned hash = 0;
  while (size_bytes > 0) {
    if (size_bytes >= sizeof(unsigned int)) {
      hash = __builtin_ia32_crc32si(hash, *(unsigned int *)obj);
      obj  = (unsigned int *)obj + 1;
      size_bytes -= sizeof(unsigned int);
    } else {
      unsigned int c = *(unsigned char *)obj;
      hash           = __builtin_ia32_crc32si(hash, c);
      obj            = (unsigned char *)obj + 1;
      size_bytes -= 1;
    }
  }
  return hash;
}

unsigned count_trailing_zeros(unsigned x) { return x == 0 ? 32 : __builtin_ctz(x); }

unsigned find_first_set_bit(unsigned x) { return x == 0 ? 0 : (unsigned)__builtin_ctz(x) + 1; }

unsigned min(unsigned a, unsigned b) { return a < b ? a : b; }

unsigned rotate_left(unsigned x, unsigned n) {
  n &= 31;
  return n == 0 ? x : (x << n) | (x >> (32 - n));
}

unsigned power_of_two(unsigned exponent) { return 1u << exponent; }

unsigned divide(unsigned numerator, unsigned denominator) { return denominator == 0 ? 0 : numerator / denominator; }

unsigned ln(unsigned x, unsigned scale) { return x == 0 ? 0 : (unsigned)(__builtin_log((double)x) * scale); }

static uint32_t reflect32(uint32_t value) {
  uint32_t reflected = 0;
  for (unsigned bit = 0; bit < 32; bit++) {
    if (value & (1u << bit)) {
      reflected |= 1u << (31 - bit);
    }
  }
  return reflected;
}

void crc32_hasher_init(struct crc32_hasher *hasher, const struct crc32_config *config) {
  hasher->config = *config;

  if (config->reversed) {
    // Reflected algorithm: the table is built with the reflected polynomial and the register shifts
    // right, which is the same as reflecting each input byte and the output of the normal form.
    const uint32_t poly = reflect32(config->coeff);
    for (uint32_t n = 0; n < 256; n++) {
      uint32_t c = n;
      for (unsigned k = 0; k < 8; k++) {
        c = (c & 1) ? (c >> 1) ^ poly : c >> 1;
      }
      hasher->table[n] = c;
    }
  } else {
    for (uint32_t n = 0; n < 256; n++) {
      uint32_t c = n << 24;
      for (unsigned k = 0; k < 8; k++) {
        c = (c & 0x80000000u) ? (c << 1) ^ config->coeff : c << 1;
      }
      hasher->table[n] = c;
    }
  }
}

uint32_t crc32_hasher_hash(const struct crc32_hasher *hasher, const void *data, size_t size) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t c           = hasher->config.init;

  if (hasher->config.reversed) {
    for (size_t i = 0; i < size; i++) {
      c = hasher->table[(c ^ bytes[i]) & 0xFF] ^ (c >> 8);
    }
  } else {
    for (size_t i = 0; i < size; i++) {
      c = hasher->table[((c >> 24) ^ bytes[i]) & 0xFF] ^ (c << 8);
    }
  }

  return c ^ hasher->config.xor_out;
}
