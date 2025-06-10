#include "lib/state/cht.h"

#include "cht-control.h"
#include "double-chain-control.h"
#include "map-control.h"
#include "vector-control.h"

#include "klee/klee.h"

struct CHT {
  struct str_field_descr key_fields[PREALLOC_SIZE];
  struct nested_field_descr key_nests[PREALLOC_SIZE];
  int key_field_count;
  int nested_key_field_count;
  int has_layout;
  int key_size;
  char *key_type;

  uint32_t height;
  uint32_t key_size;
  uint32_t backends;
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

void cht_set_layout(struct CHT *cht, struct str_field_descr *key_fields, int key_fields_count, struct nested_field_descr *key_nests,
                    int nested_key_fields_count, char *key_type) {
  // Do not trace. This function is an internal knob of the model.
  klee_assert(key_fields_count < PREALLOC_SIZE);
  klee_assert(nested_key_fields_count < PREALLOC_SIZE);
  memcpy(cht->key_fields, key_fields, sizeof(struct str_field_descr) * key_fields_count);
  if (0 < nested_key_fields_count) {
    memcpy(cht->key_nests, key_nests, sizeof(struct nested_field_descr) * nested_key_fields_count);
  }
  cht->key_field_count        = key_fields_count;
  cht->nested_key_field_count = nested_key_fields_count;
  cht->key_size               = calculate_str_size(key_fields, key_fields_count);
  klee_assert(cht->key_size < PREALLOC_SIZE);
  cht->has_layout = 1;
  cht->key_type   = key_type;
}

void cht_reset(struct CHT *cht) {}

int cht_allocate(uint32_t height, uint32_t key_size, struct CHT **cht_out) {
  klee_trace_ret();

  klee_trace_param_u32(height, "height");
  klee_trace_param_u16(key_size, "key_size");
  klee_trace_param_ptr(cht_out, sizeof(struct CHT *), "cht_out");

  int allocation_succeeded = klee_int("cht_allocation_succeeded");

  if (allocation_succeeded) {
    *cht_out = malloc(sizeof(struct CHT));
    klee_make_symbolic((*cht_out), sizeof(struct CHT), "cht");
    klee_assert((*cht_out) != NULL);
  }

  klee_assert(height > 0);
  klee_assert(key_size > 0);
  klee_assert(is_prime(height));

  (*cht_out)->height   = height;
  (*cht_out)->key_size = key_size;

  return allocation_succeeded;
}

int cht_add_backend(struct CHT *cht, int backend) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)cht, "cht");
  klee_trace_param_i32(backend, "backend");

  int success = klee_int("cht_add_backend_success");
  if (!success) {
    return 0;
  }

  return 1;
}

int cht_remove_backend(struct CHT *cht, int backend) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)cht, "cht");
  klee_trace_param_i32(backend, "backend");

  int success = klee_int("cht_remove_backend_success");
  if (!success) {
    return 0;
  }

  return 1;
}

int cht_find_backend(struct CHT *cht, void *key, int *chosen_backend_out) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)cht, "cht");
  klee_trace_param_tagged_ptr(key, cht->key_size, "key", cht->key_type, TD_BOTH);
  klee_trace_param_ptr(chosen_backend_out, sizeof(int), "chosen_backend_out");

  int success = klee_int("cht_find_backend_success");
  if (!success) {
    return 0;
  }

  *chosen_backend_out = klee_int("chosen_backend");

  return 1;
}
