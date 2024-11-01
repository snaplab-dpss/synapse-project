#include "lib/verified/cms.h"
#include "cms-control.h"
#include <klee/klee.h>
#include <stdlib.h>
#include <string.h>

#include "../verified/double-chain-control.h"
#include "../verified/map-control.h"
#include "../verified/vector-control.h"

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

unsigned find_next_power_of_2_bigger_than(uint32_t d) {
  assert(d <= 0x80000000);
  unsigned n = 1;

  while (n < d) {
    n *= 2;
  }

  return n;
}

struct str_field_descr hash_descrs[] = {
    {offsetof(struct hash, value), sizeof(uint32_t), 0, "value"},
};
struct nested_field_descr hash_nests[] = {};

bool hash_eq(void *a, void *b) {
  struct hash *id1 = (struct hash *)a;
  struct hash *id2 = (struct hash *)b;

  return (id1->value == id2->value);
}

void hash_allocate(void *obj) {
  struct hash *id = (struct hash *)obj;
  id->value = 0;
}

unsigned hash_hash(void *obj) {
  klee_trace_ret();
  klee_trace_param_tagged_ptr(obj, sizeof(struct hash), "obj", "hash", TD_BOTH);
  for (int i = 0; i < sizeof(hash_descrs) / sizeof(hash_descrs[0]); ++i) {
    klee_trace_param_ptr_field_arr_directed(
        obj, hash_descrs[i].offset, hash_descrs[i].width, hash_descrs[i].count,
        hash_descrs[i].name, TD_BOTH);
  }
  for (int i = 0; i < sizeof(hash_nests) / sizeof(hash_nests[0]); ++i) {
    klee_trace_param_ptr_nested_field_arr_directed(
        obj, hash_nests[i].base_offset, hash_nests[i].offset,
        hash_nests[i].width, hash_nests[i].count, hash_nests[i].name, TD_BOTH);
  }
  return klee_int("hash_hash");
}

struct str_field_descr bucket_descrs[] = {
    {offsetof(struct cms_bucket, value), sizeof(uint32_t), 0, "value"},
};
struct nested_field_descr bucket_nests[] = {};

bool bucket_eq(void *a, void *b) {
  struct cms_bucket *id1 = (struct cms_bucket *)a;
  struct cms_bucket *id2 = (struct cms_bucket *)b;

  return (id1->value == id2->value);
}

void bucket_allocate(void *obj) { (uintptr_t) obj; }

unsigned bucket_hash(void *obj) {
  struct cms_bucket *id = (struct cms_bucket *)obj;

  unsigned hash = 0;
  hash = __builtin_ia32_crc32si(hash, id->value);
  return hash;
}

void cms_set_layout(struct CMS *cms, struct str_field_descr *key_fields,
                    int key_fields_count, struct nested_field_descr *key_nests,
                    int nested_key_fields_count, char *key_type) {
  // Do not trace. This function is an internal knob of the model.
  klee_assert(key_fields_count < PREALLOC_SIZE);
  klee_assert(nested_key_fields_count < PREALLOC_SIZE);
  memcpy(cms->key_fields, key_fields,
         sizeof(struct str_field_descr) * key_fields_count);
  if (0 < nested_key_fields_count) {
    memcpy(cms->key_nests, key_nests,
           sizeof(struct nested_field_descr) * nested_key_fields_count);
  }
  cms->key_field_count = key_fields_count;
  cms->nested_key_field_count = nested_key_fields_count;
  cms->key_size = calculate_str_size(key_fields, key_fields_count);
  klee_assert(cms->key_size < PREALLOC_SIZE);
  cms->has_layout = 1;
  cms->key_type = key_type;
}

void cms_set_entry_condition(struct CMS *cms, cms_entry_condition *cond,
                             void *state) {}

void cms_reset(struct CMS *cms) {}

int cms_allocate(uint32_t capacity, uint16_t threshold, uint32_t key_size,
                 struct CMS **cms_out) {
  klee_trace_ret();

  klee_trace_param_u32(capacity, "capacity");
  klee_trace_param_u16(threshold, "threshold");
  klee_trace_param_u16(key_size, "key_size");
  klee_trace_param_ptr(cms_out, sizeof(struct CMS *), "cms_out");

  klee_assert(CMS_HASHES <= CMS_SALTS_BANK_SIZE);

  int allocation_succeeded = klee_int("cms_allocation_succeeded");

  if (allocation_succeeded) {
    *cms_out = malloc(sizeof(struct CMS));
    klee_make_symbolic((*cms_out), sizeof(struct CMS), "cms");
    klee_assert((*cms_out) != NULL);
    return 1;
  }

  return 0;
}

void cms_compute_hashes(struct CMS *cms, void *k) {
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_tagged_ptr(k, cms->key_size, "key", cms->key_type, TD_BOTH);
}

void cms_refresh(struct CMS *cms, time_ns_t now) {
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_u64(now, "time");
}

int cms_fetch(struct CMS *cms) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)cms, "cms");
  return klee_int("overflow");
}

int cms_touch_buckets(struct CMS *cms, time_ns_t now) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_u64(now, "time");
  return klee_int("success");
}

void cms_expire(struct CMS *cms, time_ns_t time) {
  klee_trace_param_u64((uint64_t)cms, "cms");
  klee_trace_param_i64(time, "time");

  for (int i = 0; i < CMS_HASHES; i++) {
    int nfreed = klee_int("number_of_freed_flows");
    klee_assume(0 <= nfreed);
    // dchain_make_space(cms->allocators[i], nfreed);
  }
}
