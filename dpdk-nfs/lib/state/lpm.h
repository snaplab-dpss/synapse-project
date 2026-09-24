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

// Longest-prefix match over a key of any fixed width. A 4-byte key -- an IPv4 address -- is served
// by the DIR-24-8 tables this file is named after; anything wider by a list of prefixes scanned for
// the longest one that matches, which is enough for the entry counts a lookup table carries.
int lpm_allocate(uint32_t capacity, uint32_t key_size, struct LPM **lpm_out);
void lpm_free(struct LPM *lpm);

// Fill the lpm data structure with the prefix entries.
// This is parsed from a configuration file "cfg_fname".
// The configuration file expects the following format per line: "<ipv4 addr>/<subnet size> <device>".
// E.g.:
// 10.0.0.0/8 0
// 11.0.0.0/8 1
// ...
void lpm_from_file(struct LPM *lpm, const char *cfg_fname);

// Adds a prefix of `prefixlen` bits, read from the front of `prefix`. A 4-byte key is taken in
// network byte order, as an address is.
int lpm_update(struct LPM *lpm, const void *prefix, uint32_t prefixlen, int value);

// Returns whether any prefix matches `key`, and through value_out the one that matched furthest.
int lpm_lookup(struct LPM *lpm, const void *key, int *value_out);
