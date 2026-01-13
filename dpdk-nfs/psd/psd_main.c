#include <stdint.h>
#include <assert.h>

#include <rte_byteorder.h>

#include "lib/util/expirator.h"
#include "lib/util/expirator.h"

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "config.h"
#include "state.h"
#include "ip_addr.h"
#include "counter.h"
#include "touched_port.h"

struct nf_config config;
struct State *state;

bool nf_init(void) {
  state = alloc_state();
  return state != NULL;
}

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

  return dst_dev;
}

void expire_entries(time_ns_t time) {
  assert(time >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u             = (uint64_t)time;                            // OK because of the two asserts
  uint64_t expiration_time_ns = ((uint64_t)config.expiration_time) * 1000; // us to ns
  time_ns_t last_time         = time_u - expiration_time_ns;
  expire_items_single_map(state->allocator, state->srcs_key, state->srcs, last_time);
  bf_periodic_cleanup(state->touched_ports, time);
}

int allocate(uint32_t src, uint16_t target_port, time_ns_t time) {
  int index      = -1;
  int port_index = -1;

  int allocated = dchain_allocate_new_index(state->allocator, &index, time);

  if (!allocated) {
    // Nothing we can do...
    NF_DEBUG("No more space in the Port Scanner Detector source table");
    return false;
  }

  NF_DEBUG("Allocating %3u.%3u.%3u.%3u", (src >> 0) & 0xff, (src >> 8) & 0xff, (src >> 16) & 0xff, (src >> 24) & 0xff);

  uint32_t *src_key = NULL;
  vector_borrow(state->srcs_key, index, (void **)&src_key);
  *src_key = src;
  map_put(state->srcs, src_key, index);
  vector_return(state->srcs_key, index, src_key);

  uint32_t *counter = NULL;
  vector_borrow(state->touched_ports_counter, index, (void **)&counter);
  *counter = 1;
  vector_return(state->touched_ports_counter, index, counter);

  struct TouchedPort touched_port = {.src = src, .port = target_port};
  bf_set(state->touched_ports, &touched_port);

  return true;
}

// Return true if a port scanning is detected.
int detect_port_scanning(uint32_t src, uint16_t target_port, time_ns_t time) {
  int index      = -1;
  int port_index = -1;
  int present    = map_get(state->srcs, &src, &index);

  if (!present) {
    NF_DEBUG("Allocating %3u.%3u.%3u.%3u", (src >> 0) & 0xff, (src >> 8) & 0xff, (src >> 16) & 0xff, (src >> 24) & 0xff);

    bool allocated = allocate(src, target_port, time);

    if (!allocated) {
      // Nothing we can do, the table is full...
      NF_DEBUG("No more space");
      return false;
    }

    return false;
  }

  dchain_rejuvenate_index(state->allocator, index, time);

  uint32_t *counter = NULL;
  vector_borrow(state->touched_ports_counter, index, (void **)&counter);

  struct TouchedPort touched_port = {.src = src, .port = target_port};
  int port_in_use                 = bf_query(state->touched_ports, &touched_port);
  bf_set(state->touched_ports, &touched_port);

  if (!port_in_use && *counter >= config.max_ports) {
    NF_DEBUG("Dropping   %3u.%3u.%3u.%3u", (src >> 0) & 0xff, (src >> 8) & 0xff, (src >> 16) & 0xff, (src >> 24) & 0xff);
    vector_return(state->touched_ports_counter, index, counter);
    return true;
  }

  if (!port_in_use) {
    (*counter)++;
  }

  vector_return(state->touched_ports_counter, index, counter);

  return false;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  expire_entries(now);

  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *rte_ipv4_header = nf_then_get_ipv4_header(rte_ether_header, buffer);
  if (rte_ipv4_header == NULL) {
    return DROP;
  }

  struct tcpudp_hdr *tcpudp_header = nf_then_get_tcpudp_header(rte_ipv4_header, buffer);
  if (tcpudp_header == NULL) {
    return DROP;
  }

  if (is_internal(device)) {
    // Simply forward outgoing packets.
    NF_DEBUG("Outgoing packet. Not checking for port scanning attempts.");
  } else {
    if (detect_port_scanning(rte_ipv4_header->src_addr, tcpudp_header->dst_port, now)) {
      return DROP;
    }
  }

  return get_dst_dev(device);
}
