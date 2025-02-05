#include "klee/klee.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lib/state/lpm-dir-24-8.h"
#include "lpm-dir-24-8-control.h"

#define PREALLOC_SIZE (256)
#define NUM_ELEMS (3)

struct LPM {
  lpm_entry_condition *cond;
};

int lpm_allocate(struct LPM **lpm_out) {
  klee_trace_ret();
  klee_trace_param_ptr(lpm_out, sizeof(struct LPM *), "lpm_out");
  int allocation_succeeded = klee_int("lpm_alloc_success");
  if (!allocation_succeeded)
    return 0;
  *lpm_out         = malloc(sizeof(struct LPM));
  (**lpm_out).cond = NULL;
  return 1;
}

void lpm_free(struct LPM *lpm) {
  klee_assert(0); // Not supported
}

int lpm_update(struct LPM *lpm, uint32_t prefix, uint8_t prefixlen, uint16_t value) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  klee_trace_param_u32(prefix, "prefix");
  // VV should be u8, but too lazy to add that to klee
  klee_trace_param_u16(prefixlen, "prefixlen");
  klee_trace_param_u16(value, "value");

  klee_assert(lpm != NULL);

  int success = klee_int("lpm_update_elem_result");
  if (success) {
    return 1;
  } else {
    return 0;
  }
}

int lpm_lookup(struct LPM *lpm, uint32_t prefix, uint16_t *value_out) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  klee_trace_param_u32(prefix, "prefix");
  klee_trace_param_ptr(value_out, sizeof(uint16_t), "value_out");

  klee_assert(lpm != NULL);

  int match = klee_int("lpm_lookup_match");
  if (match) {
    int value = klee_int("lpm_lookup_result");
    if (lpm->cond) {
      klee_assume(lpm->cond(prefix, value));
    }
    *value_out = value;
    return 1;
  }

  return 0;
}

void lpm_set_entry_condition(struct LPM *lpm, lpm_entry_condition *cond) { lpm->cond = cond; }

void lpm_from_file(struct LPM *lpm, const char *cfg_fname) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)lpm, "lpm");
  klee_trace_param_ptr((void *)cfg_fname, strlen(cfg_fname), "cfg_fname");
}