#include "klee/klee.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lib/state/lpm.h"
#include "lpm-control.h"

#define PREALLOC_SIZE (256)
#define NUM_ELEMS (3)

struct LPM {
  lpm_entry_condition *cond;
  uint32_t key_size;
};

int lpm_allocate(uint32_t capacity, uint32_t key_size, struct LPM **lpm_out) {
  klee_trace_ret();
  klee_trace_param_u32(capacity, "capacity");
  klee_trace_param_u32(key_size, "key_size");
  klee_trace_param_ptr(lpm_out, sizeof(struct LPM *), "lpm_out");

  int allocation_succeeded = klee_int("lpm_alloc_success");
  if (allocation_succeeded) {
    *lpm_out             = malloc(sizeof(struct LPM));
    (**lpm_out).cond     = NULL;
    (**lpm_out).key_size = key_size;
  }

  return allocation_succeeded;
}

void lpm_free(struct LPM *lpm) {
  klee_assert(0); // Not supported
}

int lpm_update(struct LPM *lpm, const void *prefix, uint32_t prefixlen, int value) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  // The prefix itself, not just where it lives: what this installs is its bytes, and anything
  // generated from the resulting description has to reproduce them.
  klee_trace_param_ptr((void *)prefix, lpm->key_size, "prefix");
  klee_trace_param_u32(prefixlen, "prefixlen");
  klee_trace_param_i32(value, "value");

  klee_assert(lpm != NULL);

  return klee_int("lpm_update_elem_result");
}

int lpm_lookup(struct LPM *lpm, const void *prefix, int *value_out) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  // The key itself, not just where it lives: what matched is a function of its bytes.
  klee_trace_param_ptr((void *)prefix, lpm->key_size, "prefix");
  klee_trace_param_ptr(value_out, sizeof(int), "value_out");

  klee_assert(lpm != NULL);

  int match = klee_int("lpm_lookup_match");
  if (match) {
    *value_out = klee_int("lpm_lookup_result");
  }

  return match;
}

void lpm_set_entry_condition(struct LPM *lpm, lpm_entry_condition *cond) { lpm->cond = cond; }

void lpm_from_file(struct LPM *lpm, const char *cfg_fname) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  klee_trace_param_ptr((void *)cfg_fname, strlen(cfg_fname), "cfg_fname");
}