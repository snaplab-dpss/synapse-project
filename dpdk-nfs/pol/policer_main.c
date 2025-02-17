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

  return dst_dev;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  tb_expire(state->tb, now);

  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);
  struct rte_ipv4_hdr *rte_ipv4_header   = nf_then_get_ipv4_header(rte_ether_header, buffer);

  if (rte_ipv4_header == NULL) {
    NF_DEBUG("Not IPv4, dropping");
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
        NF_DEBUG("Incoming packet outside of policed rate. Dropping.");
        return DROP;
      }
    }

    int allocated = tb_trace(state->tb, &rte_ipv4_header->dst_addr, packet_length, now, &index);

    if (!allocated) {
      NF_DEBUG("No tokens. Dropping.");
      return DROP;
    }
  }

  return get_dst_dev(device);
}
