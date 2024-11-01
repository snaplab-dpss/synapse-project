#include <klee/klee.h>
#include "lib/verified/expirator.h"
#include "double-chain-control.h"

int expire_items_single_map(struct DoubleChain *chain, struct Vector *vector,
                            struct Map *map, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_u64((uint64_t)vector, "vector");
  klee_trace_param_u64((uint64_t)map, "map");
  klee_trace_param_i64(time, "time");
  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);
  dchain_make_space(chain, nfreed);
  return nfreed;
}

int expire_items_single_map_iteratively(struct Vector *vector, struct Map *map,
                                        int start, int n_elems) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)vector, "vector");
  klee_trace_param_u64((uint64_t)map, "map");
  klee_trace_param_i32(start, "start");
  klee_trace_param_i32(n_elems, "n_elems");
  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);
  return nfreed;
}
