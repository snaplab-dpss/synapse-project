#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#define lpm_PLEN_MAX 32
#define BYTE_SIZE 8

#define INVALID 0xFFFF

#define lpm_24_FLAG_MASK 0x8000      // == 0b1000 0000 0000 0000
#define lpm_24_MAX_ENTRIES 16777216  //= 2^24
#define lpm_24_VAL_MASK 0x7FFF
#define lpm_24_PLEN_MAX 24

#define lpm_LONG_OFFSET_MAX 256
#define lpm_LONG_FACTOR 256
#define lpm_LONG_MAX_ENTRIES 65536  //= 2^16

#define MAX_NEXT_HOP_VALUE 0x7FFF

// http://tiny-tera.stanford.edu/~nickm/papers/Infocom98_lookup.pdf

// I assume that the rules will be in ascending order of prefixlen
// Each new rule will simply overwrite any existing rule where it should exist
// The entries in lpm_24 are as follows:
//   bit15: 0->next hop, 1->lpm_long lookup
//   bit14-0: value of next hop or index in lpm_long
//
// The entries in lpm_long are as follows:
//   bit15-0: value of next hop
//
// max next hop value is 2^15 - 1.

struct lpm;

int lpm_allocate(struct lpm **lpm_out);
void lpm_free(struct lpm *_lpm);
int lpm_update_elem(struct lpm *_lpm, uint32_t prefix, uint8_t prefixlen,
                    uint16_t value);
int lpm_lookup_elem(struct lpm *_lpm, uint32_t prefix);
