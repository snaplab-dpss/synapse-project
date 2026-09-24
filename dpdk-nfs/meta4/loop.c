#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                struct Vector **byte_counts, uint32_t drt_capacity, unsigned int lcore_id, time_ns_t *time) {
  map_reset(*drt);
  vector_reset(*drt_keys);
  vector_reset(*drt_domains);
  dchain_reset(*drt_allocator, drt_capacity);
  vector_reset(*dns_queried);
  vector_reset(*dns_missed);
  vector_reset(*pkt_counts);
  vector_reset(*byte_counts);

  *time = restart_time();
}

void loop_invariant_consume(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                            struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                            struct Vector **byte_counts, uint32_t drt_capacity, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_ptr(drt, sizeof(struct Map *), "drt");
  klee_trace_param_ptr(drt_keys, sizeof(struct Vector *), "drt_keys");
  klee_trace_param_ptr(drt_domains, sizeof(struct Vector *), "drt_domains");
  klee_trace_param_ptr(drt_allocator, sizeof(struct DoubleChain *), "drt_allocator");
  klee_trace_param_ptr(dns_queried, sizeof(struct Vector *), "dns_queried");
  klee_trace_param_ptr(dns_missed, sizeof(struct Vector *), "dns_missed");
  klee_trace_param_ptr(pkt_counts, sizeof(struct Vector *), "pkt_counts");
  klee_trace_param_ptr(byte_counts, sizeof(struct Vector *), "byte_counts");

  klee_trace_param_u32(drt_capacity, "drt_capacity");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                            struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                            struct Vector **byte_counts, uint32_t drt_capacity, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();

  klee_trace_param_ptr(drt, sizeof(struct Map *), "drt");
  klee_trace_param_ptr(drt_keys, sizeof(struct Vector *), "drt_keys");
  klee_trace_param_ptr(drt_domains, sizeof(struct Vector *), "drt_domains");
  klee_trace_param_ptr(drt_allocator, sizeof(struct DoubleChain *), "drt_allocator");
  klee_trace_param_ptr(dns_queried, sizeof(struct Vector *), "dns_queried");
  klee_trace_param_ptr(dns_missed, sizeof(struct Vector *), "dns_missed");
  klee_trace_param_ptr(pkt_counts, sizeof(struct Vector *), "pkt_counts");
  klee_trace_param_ptr(byte_counts, sizeof(struct Vector *), "byte_counts");

  klee_trace_param_u32(drt_capacity, "drt_capacity");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                           struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                           struct Vector **byte_counts, uint32_t drt_capacity, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(drt, drt_keys, drt_domains, drt_allocator, dns_queried, dns_missed, pkt_counts, byte_counts, drt_capacity,
                         lcore_id, time);
  loop_reset(drt, drt_keys, drt_domains, drt_allocator, dns_queried, dns_missed, pkt_counts, byte_counts, drt_capacity, lcore_id,
             &time);
  loop_invariant_produce(drt, drt_keys, drt_domains, drt_allocator, dns_queried, dns_missed, pkt_counts, byte_counts, drt_capacity,
                         &lcore_id, &time);
}

#endif // KLEE_VERIFICATION
