#include <stdint.h>
#include <string.h>

#include <rte_byteorder.h>

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "config.h"
#include "state.h"

#include "lib/util/math.h"

struct nf_config config;
struct State *state;

struct flow_id {
  uint32_t src;
  uint32_t dst;
};

bool nf_init(void) {
  state = alloc_state();
  return state != NULL;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  (void)packet_length;
  (void)now;
  (void)mbuf;

  struct rte_ether_hdr *ether_header = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *ipv4_header = nf_then_get_ipv4_header(ether_header, buffer);
  if (ipv4_header == NULL) {
    return DROP;
  }

  struct flow_id flow = {.src = ipv4_header->src_addr, .dst = ipv4_header->dst_addr};
  uint32_t hash       = hash_obj(&flow, sizeof(flow));

  uint32_t trailing_zeros = count_trailing_zeros(hash & config.hash_mask);
  uint32_t rank           = trailing_zeros < config.scaling ? trailing_zeros + 1 : 0;
  uint32_t estimator_id   = hash >> (32 - config.log_num_estimators);

  uint32_t *estimator;
  vector_borrow(state->estimators, estimator_id, (void **)&estimator);
  uint32_t previous = *estimator;
  if (rank > previous) {
    *estimator = rank;
    vector_return(state->estimators, estimator_id, estimator);
  } else {
    vector_return(state->estimators, estimator_id, estimator);
  }

  uint32_t shadow     = rank > previous ? previous : rank;
  uint32_t difference = power_of_two(config.scaling - shadow) - power_of_two(config.scaling - rank);

  uint32_t *sum;
  vector_borrow(state->accumulator, 0, (void **)&sum);
  *sum += difference;
  uint32_t total = *sum;
  vector_return(state->accumulator, 0, sum);

  uint32_t estimate = divide(config.magnify_factor, config.offset - total);

  uint32_t *nonzero;
  vector_borrow(state->nonzero_count, 0, (void **)&nonzero);
  uint32_t count;
  if (shadow == 0) {
    *nonzero += 1;
    count = *nonzero;
    vector_return(state->nonzero_count, 0, nonzero);
  } else {
    count = *nonzero;
    vector_return(state->nonzero_count, 0, nonzero);
  }

  if (estimate < config.lc_threshold && count < config.num_estimators) {
    uint32_t empty = config.num_estimators - count;
    estimate       = config.lc_offset - ln(empty, config.num_estimators);
  }

  struct rte_ether_addr encoded = {0};
  memcpy(&encoded, &estimate, sizeof(estimate));
  ether_header->src_addr = encoded;

  return device;
}
