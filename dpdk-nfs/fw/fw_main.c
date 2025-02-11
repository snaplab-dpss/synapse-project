#include <stdlib.h>

#include "flow.h"
#include "config.h"
#include "flowmanager.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

struct nf_config config;
struct FlowManager *flow_manager;

bool nf_init(void) {
  flow_manager = flow_manager_allocate();
  return flow_manager != NULL;
}

bool is_internal(uint16_t device) {
  bool is_int_dev;

  int *is_internal;
  vector_borrow(flow_manager->state->int_devices, device, (void **)&is_internal);
  is_int_dev = (*is_internal != 0);
  vector_return(flow_manager->state->int_devices, device, is_internal);

  return is_int_dev;
}

uint16_t get_dst_dev(uint16_t src_dev) {
  uint16_t dst_dev;

  uint16_t *destination_device;
  vector_borrow(flow_manager->state->fwd_rules, src_dev, (void **)&destination_device);
  dst_dev = *destination_device;
  vector_return(flow_manager->state->fwd_rules, src_dev, destination_device);

  return dst_dev;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  flow_manager_expire(flow_manager, now);

  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);
  struct rte_ipv4_hdr *rte_ipv4_header   = nf_then_get_ipv4_header(rte_ether_header, buffer);
  if (rte_ipv4_header == NULL) {
    NF_DEBUG("Not IPv4, dropping");
    return DROP;
  }

  struct tcpudp_hdr *tcpudp_header = nf_then_get_tcpudp_header(rte_ipv4_header, buffer);
  if (tcpudp_header == NULL) {
    NF_DEBUG("Not TCP/UDP, dropping");
    return DROP;
  }

  if (is_internal(device)) {
    // Inverse the src and dst for the "reply flow"
    struct FlowId id = {
        .src_port = tcpudp_header->dst_port,
        .dst_port = tcpudp_header->src_port,
        .src_ip   = rte_ipv4_header->dst_addr,
        .dst_ip   = rte_ipv4_header->src_addr,
        .protocol = rte_ipv4_header->next_proto_id,
    };

    if (!flow_manager_get_refresh_flow(flow_manager, &id, now)) {
      NF_DEBUG("Unknown external flow, dropping");
      return DROP;
    }
  } else {
    struct FlowId id = {
        .src_port = tcpudp_header->src_port,
        .dst_port = tcpudp_header->dst_port,
        .src_ip   = rte_ipv4_header->src_addr,
        .dst_ip   = rte_ipv4_header->dst_addr,
        .protocol = rte_ipv4_header->next_proto_id,
    };

    flow_manager_allocate_or_refresh_flow(flow_manager, &id, now);
  }

  return get_dst_dev(device);
}
