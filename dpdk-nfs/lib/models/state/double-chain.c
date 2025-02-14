#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <klee/klee.h>
#include "lib/state/double-chain.h"

#define ALLOW(chain) klee_allow_access((chain), sizeof(struct DoubleChain))
#define DENY(chain) klee_forbid_access((chain), sizeof(struct DoubleChain), "allocated_chain_do_not_dereference")

struct DoubleChain {
  int not_out_of_space;
  int new_index;
  int is_index_allocated;
};

__attribute__((noinline)) int dchain_allocate(int index_range, struct DoubleChain **chain_out) {
  klee_trace_ret();
  klee_trace_param_i32(index_range, "index_range");
  klee_trace_param_ptr_directed(chain_out, sizeof(struct DoubleChain *), "chain_out", TD_OUT);

  int is_dchain_allocated = klee_int("is_dchain_allocated");

  if (is_dchain_allocated) {
    *chain_out = malloc(sizeof(struct DoubleChain));
    memset(*chain_out, 0, sizeof(struct DoubleChain));
    (*chain_out)->new_index = klee_int("new_index");
    klee_assume(0 <= (*chain_out)->new_index);
    klee_assume((*chain_out)->new_index < index_range);
    (*chain_out)->is_index_allocated = 0;
    (*chain_out)->not_out_of_space   = klee_int("not_out_of_space");
  }

  return is_dchain_allocated;
}

__attribute__((noinline)) int dchain_allocate_new_index(struct DoubleChain *chain, int *index_out, time_ns_t time) {
  klee_trace_ret();
  // Deliberately trace this pointer as an integer to avoid
  // dereference.
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_ptr(index_out, sizeof(int), "index_out");
  klee_trace_param_u64(time, "time");

  if (chain->not_out_of_space) {
    klee_assert(!(chain->is_index_allocated));
    *index_out                = chain->new_index;
    chain->is_index_allocated = 1;
  }

  return chain->not_out_of_space;
}

__attribute__((noinline)) int dchain_rejuvenate_index(struct DoubleChain *chain, int index, time_ns_t time) {
  klee_trace_ret();
  // Deliberately trace this pointer as an integer to avoid
  // dereference.
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_i32(index, "index");
  klee_trace_param_u64(time, "time");

  klee_assert(chain != NULL);
  return 1;
}

__attribute__((noinline)) int dchain_expire_one_index(struct DoubleChain *chain, int *index_out, time_ns_t time) {
  klee_trace_ret();
  // Deliberately trace this pointer as an integer to avoid
  // dereference.
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_ptr(index_out, sizeof(int), "index_out");
  klee_trace_param_u64(time, "time");

  klee_assert(chain != NULL);
  klee_assert(0 && "not supported in this model");
  return 0;
}

__attribute__((noinline)) int dchain_is_index_allocated(struct DoubleChain *chain, int index) {
  klee_trace_ret();
  // Deliberately trace this pointer as an integer to avoid dereference
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_i32(index, "index");

  klee_assert(chain != NULL);
  return klee_int("is_index_allocated");
}

int dchain_free_index(struct DoubleChain *chain, int index) {
  klee_trace_ret();
  // Deliberately trace this pointer as an integer to avoid dereference
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_i32(index, "index");

  klee_assert(chain != NULL);
  chain->not_out_of_space = 1;

  return 1;
}

void dchain_make_space(struct DoubleChain *chain, int nfreed) {
  // Do not trace. this function is internal for the Expirator model.
  klee_assert(nfreed == 0 | chain->is_index_allocated == 0);
  chain->not_out_of_space = klee_int("not_out_of_space");
}

void dchain_reset(struct DoubleChain *chain, int index_range) {
  // Do not trace. This function is an internal knob of the model.
  chain->is_index_allocated = 0;
  chain->not_out_of_space   = klee_int("not_out_of_space");
}
