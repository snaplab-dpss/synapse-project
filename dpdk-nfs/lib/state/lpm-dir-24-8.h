#include <stdint.h>

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

struct LPM;

#define LPM_CONFIG_FNAME_LEN 512

int lpm_allocate(struct LPM **lpm_out);
void lpm_free(struct LPM *lpm);

// Fill the lpm data structure with the prefix entries.
// This is parsed from a configuration file "cfg_fname".
// The configuration file expects the following format per line: "<ipv4 addr>/<subnet size> <device>".
// E.g.:
// 10.0.0.0/8 0
// 11.0.0.0/8 1
// ...
void lpm_from_file(struct LPM *lpm, const char *cfg_fname);

// We assume that the prefix is in network byte order.
int lpm_update(struct LPM *lpm, uint32_t prefix, uint8_t prefixlen, uint16_t value);

// We assume that the address is in network byte order.
int lpm_lookup(struct LPM *lpm, uint32_t addr, uint16_t *value_out);
