#include "lib/state/cms.h"

#include "cms-control.h"
#include "double-chain-control.h"
#include "map-control.h"
#include "vector-control.h"

#include <klee/klee.h>
#include <stdlib.h>
#include <string.h>

#include "lib/util/hash.h"

static int calculate_str_size(struct str_field_descr *descr, int len) {
  int rez = 0;
  int sum = 0;
  for (int i = 0; i < len; ++i) {
    sum += descr[i].width;
    if (descr[i].offset + descr[i].width > rez)
      rez = descr[i].offset + descr[i].width;
  }
  klee_assert(rez == sum);
  return rez;
}

void cms_set_layout(struct CMS *cms, struct str_field_descr *key_fields, int key_fields_count, struct nested_field_descr *key_nests,
                    int nested_key_fields_count, char *key_type) {
  // Do not trace. This function is an internal knob of the model.
  klee_assert(key_fields_count < PREALLOC_SIZE);
  klee_assert(nested_key_fields_count < PREALLOC_SIZE);
  memcpy(cms->key_fields, key_fields, sizeof(struct str_field_descr) * key_fields_count);
  if (0 < nested_key_fields_count) {
    memcpy(cms->key_nests, key_nests, sizeof(struct nested_field_descr) * nested_key_fields_count);
  }
  cms->key_field_count        = key_fields_count;
  cms->nested_key_field_count = nested_key_fields_count;
  cms->key_size               = calculate_str_size(key_fields, key_fields_count);
  klee_assert(cms->key_size < PREALLOC_SIZE);
  cms->has_layout = 1;
  cms->key_type   = key_type;
}

void cms_set_entry_condition(struct CMS *cms, cms_entry_condition *cond, void *state) {}

void cms_reset(struct CMS *cms) {}

int cms_allocate(uint32_t height, uint32_t width, uint32_t key_size, time_ns_t cleanup_interval, struct CMS **cms_out) {
  klee_trace_ret();

  klee_trace_param_u32(height, "height");
  klee_trace_param_u32(width, "width");
  klee_trace_param_u16(key_size, "key_size");
  klee_trace_param_i64(cleanup_interval, "cleanup_interval");
  klee_trace_param_ptr(cms_out, sizeof(struct CMS *), "cms_out");

  klee_assert(height <= CMS_MAX_SALTS_BANK_SIZE);

  int allocation_succeeded = klee_int("cms_allocation_succeeded");

  if (allocation_succeeded) {
    *cms_out = malloc(sizeof(struct CMS));
    klee_make_symbolic((*cms_out), sizeof(struct CMS), "cms");
    klee_assert((*cms_out) != NULL);
  }

  return allocation_succeeded;
}

void cms_increment(struct CMS *cms, void *key) {
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_tagged_ptr(key, cms->key_size, "key", cms->key_type, TD_BOTH);
}

uint64_t cms_count_min(struct CMS *cms, void *key) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_tagged_ptr(key, cms->key_size, "key", cms->key_type, TD_BOTH);
  return klee_int("min_estimate");
}

int cms_periodic_cleanup(struct CMS *cms, time_ns_t now) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_i64(now, "time");
  return klee_int("cleanup_success");
}
