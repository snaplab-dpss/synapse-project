#pragma once

#include <stdint.h>

#include "nf.h"
#include "nf-util.h"

struct nf_config {
  // Client-server pairs tracked at once. Once full, further DNS responses go unrecorded and
  // their traffic is missed, so this bounds how much of the link can be attributed.
  uint32_t drt_capacity;

  // How long a tracked pair survives without being touched, in microseconds. Too short and live
  // sessions are dropped; too long and stale pairs keep out new ones.
  uint64_t drt_expiration_time;

  // Size of the watch list, and therefore the range of domain IDs and of every per-domain counter.
  uint32_t num_known_domains;

  // The watched names themselves, already reduced to lookup keys. Their position in this array is
  // the domain ID the counters are indexed by.
  struct {
    struct dns_name *names;
    uint32_t *prefix_bits; // how much of each pattern is spelled out rather than wildcarded
    size_t n;
  } domains;

  // Client prefixes whose traffic is left out of the accounting.
  struct {
    uint32_t *prefix;
    uint8_t *prefixlen;
    size_t n;
  } ignored_clients;
};

// Reduces a dotted domain-name pattern to the key dns_get_response() builds for a name on the wire,
// and to how many bits of that key the pattern actually spells out -- a leading "*" label is a label
// it does not. Returns false if the pattern has more labels than DNS_MAX_LABELS, an empty label, or
// a "*" anywhere but at the front.
bool domain_name_from_str(const char *str, struct dns_name *name, uint32_t *prefix_bits);
