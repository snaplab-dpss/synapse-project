#include <stdlib.h>

#include "flow.h"
#include "fwd_table.h"
#include "fw_config.h"
#include "fw_flowmanager.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

struct nf_config config;

struct FlowManager *flow_manager;

bool nf_init(void) {
  flow_manager = flow_manager_allocate(config.devices_cfg_fname, config.expiration_time, config.max_flows);
  return flow_manager != NULL;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  NF_DEBUG("It is %" PRId64, now);

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

  struct FwdEntry fwd_entry;
  if (!flow_manager_fwd_table_lookup(flow_manager, device, &fwd_entry)) {
    return DROP;
  }

  if (!fwd_entry.is_internal) {
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

  rte_ether_header->d_addr = fwd_entry.addr;

  return fwd_entry.device;
  return 1;
}
