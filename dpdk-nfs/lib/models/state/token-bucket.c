#include "lib/state/token-bucket.h"

#include "token-bucket-control.h"

#include <klee/klee.h>
#include <stdlib.h>
#include <string.h>

#include "../state/double-chain-control.h"
#include "../state/map-control.h"
#include "../state/vector-control.h"

struct TokenBucket {
  struct str_field_descr key_fields[PREALLOC_SIZE];
  struct nested_field_descr key_nests[PREALLOC_SIZE];
  int key_field_count;
  int nested_key_field_count;
  int has_layout;
  int key_size;
  char *key_type;

  uint32_t capacity;
};

struct tb_bucket {
  uint64_t tokens;
  time_ns_t time;
};

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

bool bucket_entry_condition(void *value, int index, void *state) {
  struct tb_bucket *v = value;
  return (0 <= v->time) & (v->time <= recent_time()) & (v->tokens <= 10000000000);
}

void tb_set_layout(struct TokenBucket *tb, struct str_field_descr *key_fields, int key_fields_count, struct nested_field_descr *key_nests,
                   int nested_key_fields_count, char *key_type) {
  // Do not trace. This function is an internal knob of the model.
  klee_assert(key_fields_count < PREALLOC_SIZE);
  klee_assert(nested_key_fields_count < PREALLOC_SIZE);
  memcpy(tb->key_fields, key_fields, sizeof(struct str_field_descr) * key_fields_count);
  if (0 < nested_key_fields_count) {
    memcpy(tb->key_nests, key_nests, sizeof(struct nested_field_descr) * nested_key_fields_count);
  }
  tb->key_field_count        = key_fields_count;
  tb->nested_key_field_count = nested_key_fields_count;
  tb->key_size               = calculate_str_size(key_fields, key_fields_count);
  klee_assert(tb->key_size < PREALLOC_SIZE);
  tb->has_layout = 1;
  tb->key_type   = key_type;
}

void tb_reset(struct TokenBucket *tb) {}

int tb_allocate(uint32_t capacity, uint64_t rate, uint64_t burst, uint32_t key_size, struct TokenBucket **tb_out) {
  klee_trace_ret();

  klee_trace_param_u32(capacity, "capacity");
  klee_trace_param_u64(rate, "rate");
  klee_trace_param_u64(burst, "burst");
  klee_trace_param_u64(key_size, "key_size");
  klee_trace_param_ptr(tb_out, sizeof(struct TokenBucket *), "tb_out");

  int allocation_succeeded = klee_int("tb_allocation_succeeded");

  if (allocation_succeeded) {
    *tb_out = malloc(sizeof(struct TokenBucket));
    klee_make_symbolic((*tb_out), sizeof(struct TokenBucket), "tb");
    klee_assert((*tb_out) != NULL);
    (*tb_out)->capacity = capacity;
  }

  return allocation_succeeded;
}

int tb_is_tracing(struct TokenBucket *tb, void *k, int *index_out) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)tb, "tb");
  klee_trace_param_tagged_ptr(k, tb->key_size, "key", tb->key_type, TD_BOTH);
  klee_trace_param_ptr(index_out, sizeof(int), "index_out");

  *index_out = klee_int("index_out");

  klee_assert(tb != NULL);
  return klee_int("is_tracing");
}

int tb_trace(struct TokenBucket *tb, void *k, uint16_t pkt_len, time_ns_t time, int *index_out) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)tb, "tb");
  klee_trace_param_tagged_ptr(k, tb->key_size, "key", tb->key_type, TD_BOTH);
  klee_trace_param_u16(pkt_len, "pkt_len");
  klee_trace_param_u64(time, "time");
  klee_trace_param_ptr(index_out, sizeof(int), "index_out");

  klee_assert(tb != NULL);

  int successfuly_tracing = klee_int("successfuly_tracing");
  if (successfuly_tracing) {
    *index_out = klee_int("index_out");
  }

  return successfuly_tracing;
}

int tb_update_and_check(struct TokenBucket *tb, int index, uint16_t pkt_len, time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)tb, "tb");
  klee_trace_param_i32(index, "index");
  klee_trace_param_u16(pkt_len, "pkt_len");
  klee_trace_param_u64(time, "time");

  klee_assert(tb != NULL);
  return klee_int("pass");
}

int tb_expire(struct TokenBucket *tb, time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)tb, "tb");
  klee_trace_param_u64(time, "time");

  klee_assert(tb != NULL);

  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);

  return nfreed;
}
