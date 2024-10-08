#include "nop_config.h"
#include "nf.h"
#include "nf-util.h"

struct nf_config config;

bool nf_init(void) { return true; }

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length,
               time_ns_t now, struct rte_mbuf *mbuf) {
  // Mark now as unused, we don't care about time
  (void)now;

  struct rte_ether_hdr *rte_ether_header = nf_then_get_ether_header(buffer);

  uint16_t dst_device;
  if (device == config.wan_device) {
    dst_device = config.lan_device;
    rte_ether_header->s_addr.addr_bytes[0] = 0x00;
  } else {
    // dst_device = config.wan_device;
    dst_device = config.lan_device;
    rte_ether_header->s_addr.addr_bytes[0] = 0x01;
  }

  return dst_device;
}
