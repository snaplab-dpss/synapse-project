#include "config.h"
#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "state.h"

struct State *state;
struct nf_config config;

bool nf_init(void) {
  state = alloc_state();

  if (state == NULL) {
    return false;
  }

  return true;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  // Mark now as unused, we don't care about time
  (void)now;

  return device;
}
