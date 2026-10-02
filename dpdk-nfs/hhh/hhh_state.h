#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "hhh_loop.h"

#include "lib/util/math.h"

struct State {
  struct Vector *counts8;        // packets per /8 this interval, indexed by the first octet
  struct Vector *counts16;       // packets per /16 this interval, indexed by the first two octets
  struct Vector **tables24;      // the /24 HashPipe: one table of `width` hp_slots per stage
  struct crc32_hasher **hashers; // the stages' hash functions, one polynomial each
  struct Vector *int_devices;
  struct Vector *fwd_rules;
  uint32_t stages;
  uint32_t width;
  time_ns_t interval;
  uint32_t dev_count;
};

struct State *alloc_state(uint32_t stages, uint32_t width, time_ns_t interval, uint32_t dev_count);
#endif //_STATE_H_INCLUDED_
