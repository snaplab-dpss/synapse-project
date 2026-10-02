#include <stdint.h>
#include <assert.h>

#include <rte_byteorder.h>

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"

#include "hhh_config.h"
#include "hhh_state.h"

struct nf_config config;
struct State *state;

bool is_internal(uint16_t device) {
  bool is_int_dev;

  int *is_internal;
  vector_borrow(state->int_devices, device, (void **)&is_internal);
  is_int_dev = (*is_internal != 0);
  vector_return(state->int_devices, device, is_internal);

  return is_int_dev;
}

uint16_t get_dst_dev(uint16_t src_dev) {
  uint16_t dst_dev;

  uint16_t *destination_device;
  vector_borrow(state->fwd_rules, src_dev, (void **)&destination_device);
  dst_dev = *destination_device;
  vector_return(state->fwd_rules, src_dev, destination_device);

  if (src_dev == dst_dev) {
    return DROP;
  }

  return dst_dev;
}

bool nf_init(void) {
  uint32_t stages    = config.stages;
  uint32_t width     = config.width;
  time_ns_t interval = config.interval * 1000; // us to ns
  uint32_t dev_count = rte_eth_dev_count_avail();

  state = alloc_state(stages, width, interval, dev_count);

  return state != NULL;
}

#ifdef ENABLE_LOG
// The debug build reports, once per interval and just before the tables start afresh, the
// heaviest prefixes each level holds. Reporting a prefix zeroes its count so the next rank is
// found by the same scan; the tables are cleared right after anyway.
#define TOP_REPORTED 3

void report_top_counts(const char *level, struct Vector *counts, uint32_t entries) {
  for (int rank = 1; rank <= TOP_REPORTED; rank++) {
    uint32_t best_count = 0;
    uint32_t best_key   = 0;
    for (uint32_t i = 0; i < entries; i++) {
      uint32_t *count;
      vector_borrow(counts, i, (void **)&count);
      if (*count > best_count) {
        best_count = *count;
        best_key   = i;
      }
      vector_return(counts, i, count);
    }
    if (best_count == 0) {
      return;
    }
    uint32_t *count;
    vector_borrow(counts, best_key, (void **)&count);
    *count = 0;
    vector_return(counts, best_key, count);
    NF_DEBUG("TOP %s #%d: %u.%u.%u.%u count %u", level, rank, (best_key >> 0) & 0xff, (best_key >> 8) & 0xff, (best_key >> 16) & 0xff,
             (best_key >> 24) & 0xff, best_count);
  }
}

void report_top_slots(const char *level, struct Vector **tables, uint32_t stages, uint32_t width) {
  for (int rank = 1; rank <= TOP_REPORTED; rank++) {
    uint32_t best_count = 0;
    uint32_t best_key   = 0;
    for (uint32_t s = 0; s < stages; s++) {
      for (uint32_t i = 0; i < width; i++) {
        struct hp_slot *slot;
        vector_borrow(tables[s], i, (void **)&slot);
        if (slot->count > best_count) {
          best_count = slot->count;
          best_key   = slot->key;
        }
        vector_return(tables[s], i, slot);
      }
    }
    if (best_count == 0) {
      return;
    }
    // A prefix may sit in several stages (HashPipe's duplicates); they count together.
    uint32_t total = 0;
    for (uint32_t s = 0; s < stages; s++) {
      for (uint32_t i = 0; i < width; i++) {
        struct hp_slot *slot;
        vector_borrow(tables[s], i, (void **)&slot);
        if (slot->key == best_key) {
          total += slot->count;
          slot->count = 0;
        }
        vector_return(tables[s], i, slot);
      }
    }
    NF_DEBUG("TOP %s #%d: %u.%u.%u.%u count %u", level, rank, (best_key >> 0) & 0xff, (best_key >> 8) & 0xff, (best_key >> 16) & 0xff,
             (best_key >> 24) & 0xff, total);
  }
}

static time_ns_t last_report = 0;

void report_tables(time_ns_t now) {
  if (last_report == 0) {
    last_report = now;
    return;
  }
  if (now - last_report < state->interval) {
    return;
  }
  last_report = now;
  report_top_counts("/8", state->counts8, 256);
  report_top_counts("/16", state->counts16, 65536);
  report_top_slots("/24", state->tables24, state->stages, state->width);
}
#endif // ENABLE_LOG

// Every table counts one interval at a time.
void start_interval_if_elapsed(time_ns_t now) {
  vector_periodic_clear(state->counts8, now, state->interval);
  vector_periodic_clear(state->counts16, now, state->interval);
  for (uint32_t s = 0; s < state->stages; s++) {
    vector_periodic_clear(state->tables24[s], now, state->interval);
  }
}

// The /8 and /16 prefixes are few enough to count exactly, one counter per prefix. The address is
// read as stored in the packet, so its first octets are the low bytes of `src`.
void count_short_prefixes(uint32_t src) {
  uint32_t *count8;
  vector_borrow(state->counts8, src & 0xff, (void **)&count8);
  *count8 += 1;
  vector_return(state->counts8, src & 0xff, count8);

  uint32_t *count16;
  vector_borrow(state->counts16, src & 0xffff, (void **)&count16);
  *count16 += 1;
  vector_return(state->counts16, src & 0xffff, count16);
}

// HashPipe (Sivaraman et al., SOSR '17) over the /24 prefixes: a pipeline of tables that keeps
// the heaviest prefixes and evicts the lighter ones. The prefix is always inserted at the first
// stage; whatever it displaces is carried to the next stage, where it hashes to a slot and the
// lighter of the two is carried on, so that after the last stage the lightest of the sampled
// slots is gone. A hit anywhere adds the carried count to the slot, and an empty pair is carried
// from then on.
void count_long_prefix(uint32_t src) {
  struct hp_slot carried = {.key = src & 0xffffff, .count = 1};

  for (uint32_t s = 0; s < state->stages; s++) {
    uint32_t slot = crc32_hasher_hash(state->hashers[s], &carried.key, sizeof(carried.key)) & (state->width - 1);
    vector_inc_or_swap(state->tables24[s], slot, &carried, sizeof(carried.key), sizeof(carried.count), s == 0);
  }
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *rte_ipv4_header = nf_then_get_ipv4_header(rte_ether_header, buffer);
  if (rte_ipv4_header == NULL) {
    NF_DEBUG("Not IPv4, dropping");
    return DROP;
  }

  struct tcpudp_hdr *tcpudp_header = nf_then_get_tcpudp_header(rte_ipv4_header, buffer);
  if (tcpudp_header == NULL) {
    NF_DEBUG("Not TCP/UDP, dropping");
    return DROP;
  }

#ifdef ENABLE_LOG
  report_tables(now);
#endif // ENABLE_LOG

  start_interval_if_elapsed(now);

  if (is_internal(device)) {
    // Simply forward outgoing packets.
    NF_DEBUG("Outgoing packet. Not counting.");
  } else {
    count_short_prefixes(rte_ipv4_header->src_addr);
    count_long_prefix(rte_ipv4_header->src_addr);
  }

  return get_dst_dev(device);
}
