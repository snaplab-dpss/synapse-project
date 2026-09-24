#include "state.h"
#include "session.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->known_domains = NULL;
  if (lpm_allocate(config.num_known_domains, sizeof(struct dns_name), &(ret->known_domains)) == 0) {
    return NULL;
  }

  ret->ignored_clients = NULL;
  if (lpm_allocate(config.num_known_domains, sizeof(uint32_t), &(ret->ignored_clients)) == 0) {
    return NULL;
  }

  ret->drt = NULL;
  if (map_allocate(config.drt_capacity, sizeof(struct session), &(ret->drt)) == 0) {
    return NULL;
  }

  ret->drt_keys = NULL;
  if (vector_allocate(sizeof(struct session), config.drt_capacity, &(ret->drt_keys)) == 0) {
    return NULL;
  }

  ret->drt_domains = NULL;
  if (vector_allocate(sizeof(uint32_t), config.drt_capacity, &(ret->drt_domains)) == 0) {
    return NULL;
  }

  ret->drt_allocator = NULL;
  if (dchain_allocate(config.drt_capacity, &(ret->drt_allocator)) == 0) {
    return NULL;
  }

  ret->dns_queried = NULL;
  if (vector_allocate(sizeof(uint32_t), config.num_known_domains, &(ret->dns_queried)) == 0) {
    return NULL;
  }

  ret->dns_missed = NULL;
  if (vector_allocate(sizeof(uint32_t), config.num_known_domains, &(ret->dns_missed)) == 0) {
    return NULL;
  }

  ret->pkt_counts = NULL;
  if (vector_allocate(sizeof(uint32_t), config.num_known_domains, &(ret->pkt_counts)) == 0) {
    return NULL;
  }

  ret->byte_counts = NULL;
  if (vector_allocate(sizeof(uint32_t), config.num_known_domains, &(ret->byte_counts)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->drt, session_descrs, sizeof(session_descrs) / sizeof(session_descrs[0]), session_nests,
                 sizeof(session_nests) / sizeof(session_nests[0]), "session");
  vector_set_layout(ret->drt_keys, session_descrs, sizeof(session_descrs) / sizeof(session_descrs[0]), session_nests,
                    sizeof(session_nests) / sizeof(session_nests[0]), "session");
  vector_set_layout(ret->drt_domains, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->dns_queried, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->dns_missed, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->pkt_counts, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->byte_counts, NULL, 0, NULL, 0, "uint32_t");
#endif // KLEE_VERIFICATION

  for (size_t i = 0; i < config.domains.n && i < config.num_known_domains; i++) {
    lpm_update(ret->known_domains, &config.domains.names[i], config.domains.prefix_bits[i], (int)i);
  }

  for (size_t i = 0; i < config.ignored_clients.n; i++) {
    lpm_update(ret->ignored_clients, &config.ignored_clients.prefix[i], config.ignored_clients.prefixlen[i], 1);
  }

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->drt, &allocated_nf_state->drt_keys, &allocated_nf_state->drt_domains, &allocated_nf_state->drt_allocator,
                        &allocated_nf_state->dns_queried, &allocated_nf_state->dns_missed, &allocated_nf_state->pkt_counts,
                        &allocated_nf_state->byte_counts, config.drt_capacity, lcore_id, time);
}
#endif // KLEE_VERIFICATION
