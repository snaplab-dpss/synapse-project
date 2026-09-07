#ifndef _RTE_BYTEORDER_H_INCLUDED_
#define _RTE_BYTEORDER_H_INCLUDED_

#include <inttypes.h>

// This file MUST mirror exactly the way dpdk does it, including making the ANDs
// uint16 constants

static inline uint16_t rte_cpu_to_be_16(uint16_t x) { return ((x & UINT16_C(0x00FF)) << 8) | ((x & UINT16_C(0xFF00)) >> 8); }

static inline uint16_t rte_be_to_cpu_16(uint16_t x) { return ((x & UINT16_C(0x00FF)) << 8) | ((x & UINT16_C(0xFF00)) >> 8); }

static inline uint16_t rte_bswap16(uint16_t x) { return ((x & UINT16_C(0x00FF)) << 8) | ((x & UINT16_C(0xFF00)) >> 8); }

static inline uint32_t rte_bswap32(uint32_t x) {
  return ((x & UINT32_C(0x000000FF)) << 24) | ((x & UINT32_C(0x0000FF00)) << 8) | ((x & UINT32_C(0x00FF0000)) >> 8) |
         ((x & UINT32_C(0xFF000000)) >> 24);
}

static inline uint32_t rte_cpu_to_be_32(uint32_t x) { return rte_bswap32(x); }

static inline uint32_t rte_be_to_cpu_32(uint32_t x) { return rte_bswap32(x); }

#endif //_RTE_BYTEORDER_H_INCLUDED_
