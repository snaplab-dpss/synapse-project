#include "state.h"

#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/ether.h"
#include "lib/models/unverified/token-bucket-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state(uint32_t capacity, uint64_t rate, uint64_t burst,
                          uint32_t dev_count) {
  if (allocated_nf_state != NULL) {
    return allocated_nf_state;
  }

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL) {
    return NULL;
  }

  ret->tb = NULL;
  if (tb_allocate(capacity, rate, burst, sizeof(struct ip_addr), &(ret->tb)) ==
      0) {
    return NULL;
  }

  ret->capacity = capacity;
  ret->rate = rate;
  ret->burst = burst;
  ret->dev_count = dev_count;

#ifdef KLEE_VERIFICATION
  tb_set_layout(ret->tb, ip_addr_descrs,
                sizeof(ip_addr_descrs) / sizeof(ip_addr_descrs[0]),
                ip_addr_nests, sizeof(ip_addr_nests) / sizeof(ip_addr_nests[0]),
                "ip_addr");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->tb, allocated_nf_state->dev_count,
                        lcore_id, time);
}

#endif // KLEE_VERIFICATION
