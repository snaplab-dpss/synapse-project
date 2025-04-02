#include <stdint.h>
#include <assert.h>

#include <rte_byteorder.h>

#include "lib/util/expirator.h"

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "config.h"
#include "state.h"
#include "flow.h"
#include "client.h"

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

void expire_entries(time_ns_t now) {
  assert(now >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u             = (uint64_t)now;                 // OK because of the two asserts
  uint64_t expiration_time_ns = config.expiration_time * 1000; // us to ns
  time_ns_t last_time         = time_u - expiration_time_ns;
  expire_items_single_map(state->flow_allocator, state->flows_keys, state->flows, last_time);
  cms_periodic_cleanup(state->cms, now);
}

int allocate_flow(struct flow *flow, time_ns_t time) {
  int flow_index = -1;

  int allocated = dchain_allocate_new_index(state->flow_allocator, &flow_index, time);

  if (!allocated) {
    // Nothing we can do...
    NF_DEBUG("No more space in the flow table");
    return false;
  }

  NF_DEBUG("Allocating %u.%u.%u.%u:%u => %u.%u.%u.%u:%u", (flow->src_ip >> 0) & 0xff, (flow->src_ip >> 8) & 0xff,
           (flow->src_ip >> 16) & 0xff, (flow->src_ip >> 24) & 0xff, flow->src_port, (flow->dst_ip >> 0) & 0xff, (flow->dst_ip >> 8) & 0xff,
           (flow->dst_ip >> 16) & 0xff, (flow->dst_ip >> 24) & 0xff, flow->dst_port);

  struct flow *new_flow = NULL;
  vector_borrow(state->flows_keys, flow_index, (void **)&new_flow);
  memcpy((void *)new_flow, (void *)flow, sizeof(struct flow));
  map_put(state->flows, new_flow, flow_index);
  vector_return(state->flows_keys, flow_index, new_flow);

  return true;
}

// Return false if packet should be dropped
int limit_clients(struct flow *flow, time_ns_t now) {
  int flow_index = -1;
  int present    = map_get(state->flows, flow, &flow_index);

  struct client client = {
      .src_ip = flow->src_ip,
      .dst_ip = flow->dst_ip,
  };

  if (present) {
    dchain_rejuvenate_index(state->flow_allocator, flow_index, now);
    return 1;
  }

  cms_increment(state->cms, &client);
  int count = cms_count_min(state->cms, &client);

  if (count > config.max_clients) {
    return 0;
  }

  int allocated_flow = allocate_flow(flow, now);

  if (!allocated_flow) {
    // Reached the maximum number of allowed flows.
    // Just forward and don't limit...
    return 1;
  }

  return 1;
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
    NF_DEBUG("Outgoing packet. Not limiting clients.");
  } else {
    struct flow flow = {
        .src_port = tcpudp_header->src_port,
        .dst_port = tcpudp_header->dst_port,
        .src_ip   = rte_ipv4_header->src_addr,
        .dst_ip   = rte_ipv4_header->dst_addr,
        .protocol = rte_ipv4_header->next_proto_id,
    };

    if (limit_clients(&flow, now) == 0) {
      // Drop packet.
      NF_DEBUG("Limiting   %u.%u.%u.%u:%u => %u.%u.%u.%u:%u", (flow.src_ip >> 0) & 0xff, (flow.src_ip >> 8) & 0xff,
               (flow.src_ip >> 16) & 0xff, (flow.src_ip >> 24) & 0xff, flow.src_port, (flow.dst_ip >> 0) & 0xff, (flow.dst_ip >> 8) & 0xff,
               (flow.dst_ip >> 16) & 0xff, (flow.dst_ip >> 24) & 0xff, flow.dst_port);
      return DROP;
    }
  }

  return get_dst_dev(device);
}
