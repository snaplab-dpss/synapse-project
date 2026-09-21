#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "nf.h"
#include "nf-util.h"
#include "nf-log.h"
#include "config.h"
#include "state.h"

// Out-of-profile packets are remarked to CS1 / Lower Effort (RFC 3662) rather than dropped.
#define POLICED_DSCP     8
#define IPV4_DSCP_SHIFT  2
#define IPV4_ECN_MASK    0x03

struct nf_config config;

struct State *state;

bool nf_init(void) {
  state = alloc_state(config.dyn_capacity, config.rate, config.burst, rte_eth_dev_count_avail());
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

  if (src_dev == dst_dev) {
    return DROP;
  }

  return dst_dev;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  tb_expire(state->tb, now, (time_ns_t)config.expiration_time * 1000); // us to ns

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
    // Simply forward outgoing packets.
    NF_DEBUG("Outgoing packet. Not policing.");
  } else {
    int index   = -1;
    int present = tb_is_tracing(state->tb, &rte_ipv4_header->dst_addr, &index);

    if (present) {
      int pass = tb_update_and_check(state->tb, index, packet_length, now);

      if (!pass) {
        // Mark rather than drop: a dropped packet never reaches the receiver, so a throughput
        // measurement of a policed NF measures the policer's rate instead of the forwarding
        // capacity we want to report. Remarking keeps every packet on the wire and still uses the
        // bucket's verdict, which the previous KLEE-only drop did not: it compiled the decision
        // out, leaving a synthesized program that computed `pass` and discarded it.
        NF_DEBUG("Incoming packet outside of policed rate. Marking.");
        rte_ipv4_header->type_of_service = (rte_ipv4_header->type_of_service & IPV4_ECN_MASK) | (POLICED_DSCP << IPV4_DSCP_SHIFT);
        nf_set_rte_ipv4_udptcp_checksum(rte_ipv4_header, tcpudp_header, buffer);
      }
    } else {
      int allocated = tb_trace(state->tb, &rte_ipv4_header->dst_addr, packet_length, now, &index);

      if (!allocated) {
        NF_DEBUG("No tokens. Dropping.");
        return DROP;
      }
    }
  }

  return get_dst_dev(device);
}
