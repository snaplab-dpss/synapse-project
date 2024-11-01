#include "cms.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "lib/verified/boilerplate-util.h"
#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "lib/verified/double-chain.h"
#include "lib/verified/vigor-time.h"
#include "lib/verified/hash.h"

struct internal_data {
  unsigned hashes[CMS_HASHES];
  int present[CMS_HASHES];
  int buckets_indexes[CMS_HASHES];
};

struct CMS {
  struct Map *clients;
  struct Vector *keys;
  struct Vector *buckets;
  struct DoubleChain *allocators[CMS_HASHES];

  uint32_t capacity;
  uint16_t threshold;
  uint32_t key_size;

  struct internal_data internal;
};

struct hash {
  uint32_t value;
};

struct cms_bucket {
  uint32_t value;
};

struct cms_data {
  unsigned hashes[CMS_HASHES];
  int present[CMS_HASHES];
  int buckets_indexes[CMS_HASHES];
};

static unsigned find_next_power_of_2_bigger_than(uint32_t d) {
  assert(d <= 0x80000000);
  unsigned n = 1;

  while (n < d) {
    n *= 2;
  }

  return n;
}

int cms_allocate(uint32_t capacity, uint16_t threshold, uint32_t key_size,
                 struct CMS **cms_out) {
  assert(CMS_HASHES <= CMS_SALTS_BANK_SIZE);

  struct CMS *cms_alloc = (struct CMS *)malloc(sizeof(struct CMS));
  if (cms_alloc == NULL) {
    return 0;
  }

  (*cms_out) = cms_alloc;

  (*cms_out)->capacity = capacity;
  (*cms_out)->threshold = threshold;
  (*cms_out)->key_size = key_size;

  unsigned total_cms_capacity =
      find_next_power_of_2_bigger_than(capacity * CMS_HASHES);

  (*cms_out)->clients = NULL;
  if (map_allocate(total_cms_capacity, sizeof(struct hash),
                   &((*cms_out)->clients)) == 0) {
    return 0;
  }

  (*cms_out)->keys = NULL;
  if (vector_allocate(sizeof(struct hash), total_cms_capacity,
                      &((*cms_out)->keys)) == 0) {
    return 0;
  }

  (*cms_out)->buckets = NULL;
  if (vector_allocate(sizeof(struct cms_bucket), total_cms_capacity,
                      &((*cms_out)->buckets)) == 0) {
    return 0;
  }

  for (int i = 0; i < CMS_HASHES; i++) {
    (*cms_out)->allocators[i] = NULL;
    if (dchain_allocate(capacity, &((*cms_out)->allocators[i])) == 0) {
      return 0;
    }
  }

  return 1;
}

void cms_compute_hashes(struct CMS *cms, void *key) {
  for (int i = 0; i < CMS_HASHES; i++) {
    cms->internal.buckets_indexes[i] = -1;
    cms->internal.present[i] = 0;
    cms->internal.hashes[i] = 0;

    cms->internal.hashes[i] =
        __builtin_ia32_crc32si(cms->internal.hashes[i], CMS_SALTS[i]);
    cms->internal.hashes[i] = __builtin_ia32_crc32si(
        cms->internal.hashes[i], hash_obj(key, cms->key_size));
    cms->internal.hashes[i] %= cms->capacity;
  }
}

void cms_refresh(struct CMS *cms, time_ns_t now) {
  for (int i = 0; i < CMS_HASHES; i++) {
    map_get(cms->clients, &cms->internal.hashes[i],
            &cms->internal.buckets_indexes[i]);
    dchain_rejuvenate_index(cms->allocators[i],
                            cms->internal.buckets_indexes[i], now);
  }
}

int cms_fetch(struct CMS *cms) {
  int bucket_min_set = false;
  uint32_t *buckets_values[CMS_HASHES];
  uint32_t bucket_min = 0;

  for (int i = 0; i < CMS_HASHES; i++) {
    cms->internal.present[i] = map_get(cms->clients, &cms->internal.hashes[i],
                                       &cms->internal.buckets_indexes[i]);

    if (!cms->internal.present[i]) {
      continue;
    }

    int offseted = cms->internal.buckets_indexes[i] + cms->capacity * i;
    vector_borrow(cms->buckets, offseted, (void **)&buckets_values[i]);

    if (!bucket_min_set || bucket_min > *buckets_values[i]) {
      bucket_min = *buckets_values[i];
      bucket_min_set = true;
    }

    vector_return(cms->buckets, offseted, buckets_values[i]);
  }

  return bucket_min_set && bucket_min > cms->threshold;
}

int cms_touch_buckets(struct CMS *cms, time_ns_t now) {
  for (int i = 0; i < CMS_HASHES; i++) {
    int bucket_index = -1;
    int present =
        map_get(cms->clients, &cms->internal.hashes[i], &bucket_index);

    if (!present) {
      int allocated_client =
          dchain_allocate_new_index(cms->allocators[i], &bucket_index, now);

      if (!allocated_client) {
        // CMS size limit reached.
        return false;
      }

      int offseted = bucket_index + cms->capacity * i;

      uint32_t *saved_hash = 0;
      uint32_t *saved_bucket = 0;

      vector_borrow(cms->keys, offseted, (void **)&saved_hash);
      vector_borrow(cms->buckets, offseted, (void **)&saved_bucket);

      (*saved_hash) = cms->internal.hashes[i];
      (*saved_bucket) = 0;
      map_put(cms->clients, saved_hash, bucket_index);

      vector_return(cms->keys, offseted, saved_hash);
      vector_return(cms->buckets, offseted, saved_bucket);
    } else {
      dchain_rejuvenate_index(cms->allocators[i], bucket_index, now);
      uint32_t *bucket;
      int offseted = bucket_index + cms->capacity * i;
      vector_borrow(cms->buckets, offseted, (void **)&bucket);
      (*bucket)++;
      vector_return(cms->buckets, offseted, bucket);
    }
  }

  return true;
}

void cms_expire(struct CMS *cms, time_ns_t time) {
  int offset = 0;
  int index = -1;

  for (int i = 0; i < CMS_HASHES; i++) {
    offset = i * cms->capacity;

    while (dchain_expire_one_index(cms->allocators[i], &index, time)) {
      void *key;
      vector_borrow(cms->keys, index + offset, &key);
      map_erase(cms->clients, key, &key);
      vector_return(cms->keys, index + offset, key);
    }
  }
}