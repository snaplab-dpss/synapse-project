#ifndef _COMPUTE_H_INCLUDED_
#define _COMPUTE_H_INCLUDED_

#include <stdint.h>

static inline uint64_t ensure_power_of_two(uint64_t n) {
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  n++;
  return n;
}

#endif