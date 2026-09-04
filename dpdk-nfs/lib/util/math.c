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

unsigned power_of_two(unsigned exponent) { return 1u << exponent; }

unsigned divide(unsigned numerator, unsigned denominator) { return denominator == 0 ? 0 : numerator / denominator; }

unsigned ln(unsigned x, unsigned scale) { return x == 0 ? 0 : (unsigned)(__builtin_log((double)x) * scale); }
