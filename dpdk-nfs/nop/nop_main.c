#include "config.h"
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

  return true;
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
  // Mark now as unused, we don't care about time
  (void)now;

  return get_dst_dev(device);
}
