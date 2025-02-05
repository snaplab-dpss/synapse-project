#include "nop_config.h"
#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "state.h"

#include <rte_ethdev.h>

struct State *state;
struct nf_config config;

bool nf_init(void) {
  state = alloc_state();

  if (state == NULL) {
    return false;
  }

  lpm_from_file(state->fwd, "lpm.cfg");

  return true;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  // Mark now as unused, we don't care about time
  (void)now;

  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);
  struct rte_ipv4_hdr *rte_ipv4_header   = nf_then_get_ipv4_header(rte_ether_header, buffer);
  if (rte_ipv4_header == NULL) {
    NF_DEBUG("Not IPv4, dropping");
    return DROP;
  }

  NF_DEBUG("Running lpm on %u.%u.%u.%u", (rte_ipv4_header->src_addr >> 0) & 0xff, (rte_ipv4_header->src_addr >> 8) & 0xff,
           (rte_ipv4_header->src_addr >> 16) & 0xff, (rte_ipv4_header->src_addr >> 24) & 0xff);

  uint16_t dst_device;
  if (!lpm_lookup(state->fwd, rte_ipv4_header->src_addr, &dst_device)) {
    NF_DEBUG("No route found, dropping");
    return DROP;
  }

  NF_DEBUG("Sending to %u", dst_device);
  return dst_device;
}
