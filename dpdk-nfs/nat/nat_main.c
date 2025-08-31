#include <stdlib.h>

#include "nf.h"
#include "flow.h"
#include "flowmanager.h"
#include "config.h"
#include "nf-log.h"
#include "nf-util.h"

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

  if (src_dev == dst_dev) {
    return DROP;
  }

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

  NF_DEBUG("Forwarding an IPv4 packet on device %" PRIu16, device);

  if (is_internal(device)) {
    struct FlowId id = {
        .src_port = tcpudp_header->src_port,
        .dst_port = tcpudp_header->dst_port,
        .src_ip   = rte_ipv4_header->src_addr,
        .dst_ip   = rte_ipv4_header->dst_addr,
        .protocol = rte_ipv4_header->next_proto_id,
    };

    NF_DEBUG("Device %" PRIu16 " is internal", device);

    uint16_t external_port;
    if (!flow_manager_get_internal(flow_manager, &id, now, &external_port)) {
      NF_DEBUG("New flow");

      if (!flow_manager_allocate_flow(flow_manager, &id, now, &external_port)) {
        NF_DEBUG("No space for the flow, dropping");
        return DROP;
      }
    }

    NF_DEBUG("Forwarding from ext port:%d", external_port);

    rte_ipv4_header->src_addr = config.external_addr;
    tcpudp_header->src_port   = external_port;
  } else {
    NF_DEBUG("Device %" PRIu16 " is external", device);

    struct FlowId internal_flow;
    if (flow_manager_get_external(flow_manager, tcpudp_header->dst_port, now, &internal_flow)) {
      NF_DEBUG("Found internal flow.");
      LOG_FLOWID(&internal_flow, NF_DEBUG);

      if (internal_flow.dst_ip != rte_ipv4_header->src_addr | internal_flow.dst_port != tcpudp_header->src_port |
          internal_flow.protocol != rte_ipv4_header->next_proto_id) {
        NF_DEBUG("Spoofing attempt, dropping.");
        return DROP;
      }

      rte_ipv4_header->dst_addr = internal_flow.src_ip;
      tcpudp_header->dst_port   = internal_flow.src_port;
    } else {
      NF_DEBUG("Unknown flow, dropping");
      return DROP;
    }
  }

  nf_set_rte_ipv4_udptcp_checksum(rte_ipv4_header, tcpudp_header, buffer);

  return get_dst_dev(device);
}
