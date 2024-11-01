#include <arpa/inet.h>
#include <unistd.h>

#include <bf_rt/bf_rt.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <bf_switchd/bf_switchd.h>
#include <bfutils/bf_utils.h> // required for bfshell
#include <pkt_mgr/pkt_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#define ALL_PIPES 0xffff

#define ENV_VAR_SDE_INSTALL "SDE_INSTALL"
#define PROGRAM_NAME "tofino"

#define SWITCH_PACKET_MAX_BUFFER_SIZE 10000
#define MTU 1500

/**********************************************
 *
 *                   LIBVIG
 *
 **********************************************/

struct tcpudp_hdr {
  uint16_t src_port;
  uint16_t dst_port;
} __attribute__((__packed__));

#define AND &&
#define time_ns_t int64_t

time_ns_t current_time(void) {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000000000ul + tp.tv_nsec;
}

typedef unsigned map_key_hash(void *k1);
typedef bool map_keys_equality(void *k1, void *k2);

struct Map {
  int *busybits;
  void **keyps;
  unsigned *khs;
  int *chns;
  int *vals;
  unsigned capacity;
  unsigned size;
  map_keys_equality *keys_eq;
  map_key_hash *khash;
};

static unsigned loop(unsigned k, unsigned capacity) {
  return k & (capacity - 1);
}

static int find_key(int *busybits, void **keyps, unsigned *k_hashes, int *chns,
                    void *keyp, map_keys_equality *eq, unsigned key_hash,
                    unsigned capacity) {
  unsigned start = loop(key_hash, capacity);
  unsigned i = 0;
  for (; i < capacity; ++i) {
    unsigned index = loop(start + i, capacity);
    int bb = busybits[index];
    unsigned kh = k_hashes[index];
    int chn = chns[index];
    void *kp = keyps[index];
    if (bb != 0 && kh == key_hash) {
      if (eq(kp, keyp)) {
        return (int)index;
      }
    } else {
      if (chn == 0) {
        return -1;
      }
    }
  }

  return -1;
}

static unsigned find_key_remove_chain(int *busybits, void **keyps,
                                      unsigned *k_hashes, int *chns, void *keyp,
                                      map_keys_equality *eq, unsigned key_hash,
                                      unsigned capacity, void **keyp_out) {
  unsigned i = 0;
  unsigned start = loop(key_hash, capacity);

  for (; i < capacity; ++i) {
    unsigned index = loop(start + i, capacity);
    int bb = busybits[index];
    unsigned kh = k_hashes[index];
    int chn = chns[index];
    void *kp = keyps[index];
    if (bb != 0 && kh == key_hash) {
      if (eq(kp, keyp)) {
        busybits[index] = 0;
        *keyp_out = keyps[index];
        return index;
      }
    }

    chns[index] = chn - 1;
  }

  return -1;
}

static unsigned find_empty(int *busybits, int *chns, unsigned start,
                           unsigned capacity) {
  unsigned i = 0;
  for (; i < capacity; ++i) {
    unsigned index = loop(start + i, capacity);

    int bb = busybits[index];
    if (0 == bb) {
      return index;
    }
    int chn = chns[index];

    chns[index] = chn + 1;
  }

  return -1;
}

void map_impl_init(int *busybits, map_keys_equality *eq, void **keyps,
                   unsigned *khs, int *chns, int *vals, unsigned capacity) {
  (uintptr_t) eq;
  (uintptr_t) keyps;
  (uintptr_t) khs;
  (uintptr_t) vals;

  unsigned i = 0;
  for (; i < capacity; ++i) {
    busybits[i] = 0;
    chns[i] = 0;
  }
}

int map_impl_get(int *busybits, void **keyps, unsigned *k_hashes, int *chns,
                 int *values, void *keyp, map_keys_equality *eq, unsigned hash,
                 int *value, unsigned capacity) {
  int index =
      find_key(busybits, keyps, k_hashes, chns, keyp, eq, hash, capacity);

  if (-1 == index) {
    return 0;
  }

  *value = values[index];

  return 1;
}

void map_impl_put(int *busybits, void **keyps, unsigned *k_hashes, int *chns,
                  int *values, void *keyp, unsigned hash, int value,
                  unsigned capacity) {
  unsigned start = loop(hash, capacity);
  unsigned index = find_empty(busybits, chns, start, capacity);

  busybits[index] = 1;
  keyps[index] = keyp;
  k_hashes[index] = hash;
  values[index] = value;
}

void map_impl_erase(int *busybits, void **keyps, unsigned *k_hashes, int *chns,
                    void *keyp, map_keys_equality *eq, unsigned hash,
                    unsigned capacity, void **keyp_out) {
  find_key_remove_chain(busybits, keyps, k_hashes, chns, keyp, eq, hash,
                        capacity, keyp_out);
}

unsigned map_impl_size(int *busybits, unsigned capacity) {
  unsigned s = 0;
  unsigned i = 0;
  for (; i < capacity; ++i) {
    if (busybits[i] != 0) {
      ++s;
    }
  }

  return s;
}

int map_allocate(map_keys_equality *keq, map_key_hash *khash, unsigned capacity,
                 struct Map **map_out) {
  struct Map *old_map_val = *map_out;
  struct Map *map_alloc = (struct Map *)malloc(sizeof(struct Map));
  if (map_alloc == NULL)
    return 0;
  *map_out = (struct Map *)map_alloc;
  int *bbs_alloc = (int *)malloc(sizeof(int) * (int)capacity);
  if (bbs_alloc == NULL) {
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->busybits = bbs_alloc;
  void **keyps_alloc = (void **)malloc(sizeof(void *) * (int)capacity);
  if (keyps_alloc == NULL) {
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->keyps = keyps_alloc;
  unsigned *khs_alloc = (unsigned *)malloc(sizeof(unsigned) * (int)capacity);
  if (khs_alloc == NULL) {
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->khs = khs_alloc;
  int *chns_alloc = (int *)malloc(sizeof(int) * (int)capacity);
  if (chns_alloc == NULL) {
    free(khs_alloc);
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->chns = chns_alloc;
  int *vals_alloc = (int *)malloc(sizeof(int) * (int)capacity);

  if (vals_alloc == NULL) {
    free(chns_alloc);
    free(khs_alloc);
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }

  (*map_out)->vals = vals_alloc;
  (*map_out)->capacity = capacity;
  (*map_out)->size = 0;
  (*map_out)->keys_eq = keq;
  (*map_out)->khash = khash;

  map_impl_init((*map_out)->busybits, keq, (*map_out)->keyps, (*map_out)->khs,
                (*map_out)->chns, (*map_out)->vals, capacity);
  return 1;
}

int map_get(struct Map *map, void *key, int *value_out) {
  map_key_hash *khash = map->khash;
  unsigned hash = khash(key);
  return map_impl_get(map->busybits, map->keyps, map->khs, map->chns, map->vals,
                      key, map->keys_eq, hash, value_out, map->capacity);
}

void map_put(struct Map *map, void *key, int value) {
  map_key_hash *khash = map->khash;
  unsigned hash = khash(key);
  map_impl_put(map->busybits, map->keyps, map->khs, map->chns, map->vals, key,
               hash, value, map->capacity);
  ++map->size;
}

void map_erase(struct Map *map, void *key, void **trash) {
  map_key_hash *khash = map->khash;
  unsigned hash = khash(key);
  map_impl_erase(map->busybits, map->keyps, map->khs, map->chns, key,
                 map->keys_eq, hash, map->capacity, trash);

  --map->size;
}

unsigned map_size(struct Map *map) { return map->size; }

// Makes sure the allocator structur fits into memory, and particularly into
// 32 bit address space.
#define IRANG_LIMIT (1048576)

// kinda hacky, but makes the proof independent of time_ns_t... sort of
#define malloc_block_time malloc_block_llongs
#define time_integer llong_integer
#define times llongs

#define DCHAIN_RESERVED (2)

struct dchain_cell {
  int prev;
  int next;
};

struct DoubleChain {
  struct dchain_cell *cells;
  time_ns_t *timestamps;
};

enum DCHAIN_ENUM {
  ALLOC_LIST_HEAD = 0,
  FREE_LIST_HEAD = 1,
  INDEX_SHIFT = DCHAIN_RESERVED
};

void dchain_impl_init(struct dchain_cell *cells, int size) {
  struct dchain_cell *al_head = cells + ALLOC_LIST_HEAD;
  al_head->prev = 0;
  al_head->next = 0;
  int i = INDEX_SHIFT;

  struct dchain_cell *fl_head = cells + FREE_LIST_HEAD;
  fl_head->next = i;
  fl_head->prev = fl_head->next;

  while (i < (size + INDEX_SHIFT - 1)) {
    struct dchain_cell *current = cells + i;
    current->next = i + 1;
    current->prev = current->next;

    ++i;
  }

  struct dchain_cell *last = cells + i;
  last->next = FREE_LIST_HEAD;
  last->prev = last->next;
}

int dchain_impl_allocate_new_index(struct dchain_cell *cells, int *index) {
  struct dchain_cell *fl_head = cells + FREE_LIST_HEAD;
  struct dchain_cell *al_head = cells + ALLOC_LIST_HEAD;
  int allocated = fl_head->next;
  if (allocated == FREE_LIST_HEAD) {
    return 0;
  }

  struct dchain_cell *allocp = cells + allocated;
  // Extract the link from the "empty" chain.
  fl_head->next = allocp->next;
  fl_head->prev = fl_head->next;

  // Add the link to the "new"-end "alloc" chain.
  allocp->next = ALLOC_LIST_HEAD;
  allocp->prev = al_head->prev;

  struct dchain_cell *alloc_head_prevp = cells + al_head->prev;
  alloc_head_prevp->next = allocated;
  al_head->prev = allocated;

  *index = allocated - INDEX_SHIFT;

  return 1;
}

int dchain_impl_free_index(struct dchain_cell *cells, int index) {
  int freed = index + INDEX_SHIFT;

  struct dchain_cell *freedp = cells + freed;
  int freed_prev = freedp->prev;
  int freed_next = freedp->next;

  // The index is already free.
  if (freed_next == freed_prev) {
    if (freed_prev != ALLOC_LIST_HEAD) {
      return 0;
    }
  }

  struct dchain_cell *fr_head = cells + FREE_LIST_HEAD;
  struct dchain_cell *freed_prevp = cells + freed_prev;
  freed_prevp->next = freed_next;

  struct dchain_cell *freed_nextp = cells + freed_next;
  freed_nextp->prev = freed_prev;

  freedp->next = fr_head->next;
  freedp->prev = freedp->next;

  fr_head->next = freed;
  fr_head->prev = fr_head->next;

  return 1;
}

int dchain_impl_get_oldest_index(struct dchain_cell *cells, int *index) {
  struct dchain_cell *al_head = cells + ALLOC_LIST_HEAD;

  // No allocated indexes.
  if (al_head->next == ALLOC_LIST_HEAD) {
    return 0;
  }

  *index = al_head->next - INDEX_SHIFT;

  return 1;
}

int dchain_impl_rejuvenate_index(struct dchain_cell *cells, int index) {
  int lifted = index + INDEX_SHIFT;

  struct dchain_cell *liftedp = cells + lifted;
  int lifted_next = liftedp->next;
  int lifted_prev = liftedp->prev;

  if (lifted_next == lifted_prev) {
    if (lifted_next != ALLOC_LIST_HEAD) {
      return 0;
    } else {
      return 1;
    }
  }

  struct dchain_cell *lifted_prevp = cells + lifted_prev;
  lifted_prevp->next = lifted_next;

  struct dchain_cell *lifted_nextp = cells + lifted_next;
  lifted_nextp->prev = lifted_prev;

  struct dchain_cell *al_head = cells + ALLOC_LIST_HEAD;
  int al_head_prev = al_head->prev;

  liftedp->next = ALLOC_LIST_HEAD;
  liftedp->prev = al_head_prev;

  struct dchain_cell *al_head_prevp = cells + al_head_prev;
  al_head_prevp->next = lifted;

  al_head->prev = lifted;
  return 1;
}

int dchain_impl_is_index_allocated(struct dchain_cell *cells, int index) {
  int lifted = index + INDEX_SHIFT;

  struct dchain_cell *liftedp = cells + lifted;
  int lifted_next = liftedp->next;
  int lifted_prev = liftedp->prev;

  int result;
  if (lifted_next == lifted_prev) {
    if (lifted_next != ALLOC_LIST_HEAD) {
      return 0;
    } else {
      return 1;
    }
  } else {
    return 1;
  }
}

int dchain_allocate(int index_range, struct DoubleChain **chain_out) {
  struct DoubleChain *old_chain_out = *chain_out;
  struct DoubleChain *chain_alloc =
      (struct DoubleChain *)malloc(sizeof(struct DoubleChain));
  if (chain_alloc == NULL)
    return 0;
  *chain_out = (struct DoubleChain *)chain_alloc;

  struct dchain_cell *cells_alloc = (struct dchain_cell *)malloc(
      sizeof(struct dchain_cell) * (index_range + DCHAIN_RESERVED));
  if (cells_alloc == NULL) {
    free(chain_alloc);
    *chain_out = old_chain_out;
    return 0;
  }
  (*chain_out)->cells = cells_alloc;

  time_ns_t *timestamps_alloc =
      (time_ns_t *)malloc(sizeof(time_ns_t) * (index_range));
  if (timestamps_alloc == NULL) {
    free((void *)cells_alloc);
    free(chain_alloc);
    *chain_out = old_chain_out;
    return 0;
  }
  (*chain_out)->timestamps = timestamps_alloc;

  dchain_impl_init((*chain_out)->cells, index_range);

  return 1;
}

int dchain_allocate_new_index(struct DoubleChain *chain, int *index_out,
                              time_ns_t time) {
  int ret = dchain_impl_allocate_new_index(chain->cells, index_out);

  if (ret) {
    chain->timestamps[*index_out] = time;
  }

  return ret;
}

int dchain_rejuvenate_index(struct DoubleChain *chain, int index,
                            time_ns_t time) {
  int ret = dchain_impl_rejuvenate_index(chain->cells, index);

  if (ret) {
    chain->timestamps[index] = time;
  }

  return ret;
}

int dchain_expire_one_index(struct DoubleChain *chain, int *index_out,
                            time_ns_t time) {
  int has_ind = dchain_impl_get_oldest_index(chain->cells, index_out);

  if (has_ind) {
    if (chain->timestamps[*index_out] < time) {
      int rez = dchain_impl_free_index(chain->cells, *index_out);
      return rez;
    }
  }

  return 0;
}

int dchain_is_index_allocated(struct DoubleChain *chain, int index) {
  return dchain_impl_is_index_allocated(chain->cells, index);
}

int dchain_free_index(struct DoubleChain *chain, int index) {
  return dchain_impl_free_index(chain->cells, index);
}

#define VECTOR_CAPACITY_UPPER_LIMIT 140000

typedef void vector_init_elem(void *elem);

struct Vector {
  char *data;
  int elem_size;
  unsigned capacity;
};

int vector_allocate(int elem_size, unsigned capacity,
                    vector_init_elem *init_elem, struct Vector **vector_out) {
  struct Vector *old_vector_val = *vector_out;
  struct Vector *vector_alloc = (struct Vector *)malloc(sizeof(struct Vector));
  if (vector_alloc == 0)
    return 0;
  *vector_out = (struct Vector *)vector_alloc;

  char *data_alloc = (char *)malloc((uint32_t)elem_size * capacity);
  if (data_alloc == 0) {
    free(vector_alloc);
    *vector_out = old_vector_val;
    return 0;
  }
  (*vector_out)->data = data_alloc;
  (*vector_out)->elem_size = elem_size;
  (*vector_out)->capacity = capacity;

  for (unsigned i = 0; i < capacity; ++i) {
    init_elem((*vector_out)->data + elem_size * (int)i);
  }

  return 1;
}

void vector_borrow(struct Vector *vector, int index, void **val_out) {
  *val_out = vector->data + index * vector->elem_size;
}

void vector_return(struct Vector *vector, int index, void *value) {}

int expire_items_single_map(struct DoubleChain *chain, struct Vector *vector,
                            struct Map *map, time_ns_t time) {
  int count = 0;
  int index = -1;

  while (dchain_expire_one_index(chain, &index, time)) {
    void *key;
    vector_borrow(vector, index, &key);
    map_erase(map, key, &key);
    vector_return(vector, index, key);

    ++count;
  }

  return count;
}

void expire_items_single_map_iteratively(struct Vector *vector, struct Map *map,
                                         int start, int n_elems) {
  assert(start >= 0);
  assert(n_elems >= 0);
  void *key;
  for (int i = start; i < n_elems; i++) {
    vector_borrow(vector, i, (void **)&key);
    map_erase(map, key, (void **)&key);
    vector_return(vector, i, key);
  }
}

// Careful: SKETCH_HASHES needs to be <= SKETCH_SALTS_BANK_SIZE
#define SKETCH_HASHES 5
#define SKETCH_SALTS_BANK_SIZE 64

struct internal_data {
  unsigned hashes[SKETCH_HASHES];
  int present[SKETCH_HASHES];
  int buckets_indexes[SKETCH_HASHES];
};

static const uint32_t SKETCH_SALTS[SKETCH_SALTS_BANK_SIZE] = {
    0x9b78350f, 0x9bcf144c, 0x8ab29a3e, 0x34d48bf5, 0x78e47449, 0xd6e4af1d,
    0x32ed75e2, 0xb1eb5a08, 0x9cc7fbdf, 0x65b811ea, 0x41fd5ed9, 0x2e6a6782,
    0x3549661d, 0xbb211240, 0x78daa2ae, 0x8ce2d11f, 0x52911493, 0xc2497bd5,
    0x83c232dd, 0x3e413e9f, 0x8831d191, 0x6770ac67, 0xcd1c9141, 0xad35861a,
    0xb79cd83d, 0xce3ec91f, 0x360942d1, 0x905000fa, 0x28bb469a, 0xdb239a17,
    0x615cf3ae, 0xec9f7807, 0x271dcc3c, 0x47b98e44, 0x33ff4a71, 0x02a063f8,
    0xb051ebf2, 0x6f938d98, 0x2279abc3, 0xd55b01db, 0xaa99e301, 0x95d0587c,
    0xaee8684e, 0x24574971, 0x4b1e79a6, 0x4a646938, 0xa68d67f4, 0xb87839e6,
    0x8e3d388b, 0xed2af964, 0x541b83e3, 0xcb7fc8da, 0xe1140f8c, 0xe9724fd6,
    0x616a78fa, 0x610cd51c, 0x10f9173e, 0x8e180857, 0xa8f0b843, 0xd429a973,
    0xceee91e5, 0x1d4c6b18, 0x2a80e6df, 0x396f4d23,
};

struct CMS {
  struct Map *clients;
  struct Vector *keys;
  struct Vector *buckets;
  struct DoubleChain *allocators[SKETCH_HASHES];

  uint32_t capacity;
  uint16_t threshold;

  map_key_hash *kh;
  struct internal_data internal;
};

struct hash {
  uint32_t value;
};

struct bucket {
  uint32_t value;
};

struct cms_data {
  unsigned hashes[SKETCH_HASHES];
  int present[SKETCH_HASHES];
  int buckets_indexes[SKETCH_HASHES];
};

unsigned find_next_power_of_2_bigger_than(uint32_t d) {
  assert(d <= 0x80000000);
  unsigned n = 1;

  while (n < d) {
    n *= 2;
  }

  return n;
}

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
  struct hash *id = (struct hash *)obj;

  unsigned hash = 0;
  hash = __builtin_ia32_crc32si(hash, id->value);
  return hash;
}

void bucket_allocate(void *obj) { (uintptr_t) obj; }

int cms_allocate(map_key_hash *kh, uint32_t capacity, uint16_t threshold,
                 struct CMS **cms_out) {
  assert(SKETCH_HASHES <= SKETCH_SALTS_BANK_SIZE);

  struct CMS *cms_alloc = (struct CMS *)malloc(sizeof(struct CMS));
  if (cms_alloc == NULL) {
    return 0;
  }

  (*cms_out) = cms_alloc;

  (*cms_out)->capacity = capacity;
  (*cms_out)->threshold = threshold;
  (*cms_out)->kh = kh;

  unsigned total_cms_capacity =
      find_next_power_of_2_bigger_than(capacity * SKETCH_HASHES);

  (*cms_out)->clients = NULL;
  if (map_allocate(hash_eq, hash_hash, total_cms_capacity,
                   &((*cms_out)->clients)) == 0) {
    return 0;
  }

  (*cms_out)->keys = NULL;
  if (vector_allocate(sizeof(struct hash), total_cms_capacity, hash_allocate,
                      &((*cms_out)->keys)) == 0) {
    return 0;
  }

  (*cms_out)->buckets = NULL;
  if (vector_allocate(sizeof(struct bucket), total_cms_capacity,
                      bucket_allocate, &((*cms_out)->buckets)) == 0) {
    return 0;
  }

  for (int i = 0; i < SKETCH_HASHES; i++) {
    (*cms_out)->allocators[i] = NULL;
    if (dchain_allocate(capacity, &((*cms_out)->allocators[i])) == 0) {
      return 0;
    }
  }

  return 1;
}

void cms_compute_hashes(struct CMS *cms, void *key) {
  for (int i = 0; i < SKETCH_HASHES; i++) {
    cms->internal.buckets_indexes[i] = -1;
    cms->internal.present[i] = 0;
    cms->internal.hashes[i] = 0;

    cms->internal.hashes[i] =
        __builtin_ia32_crc32si(cms->internal.hashes[i], SKETCH_SALTS[i]);
    cms->internal.hashes[i] =
        __builtin_ia32_crc32si(cms->internal.hashes[i], cms->kh(key));
    cms->internal.hashes[i] %= cms->capacity;
  }
}

void cms_refresh(struct CMS *cms, time_ns_t now) {
  for (int i = 0; i < SKETCH_HASHES; i++) {
    map_get(cms->clients, &cms->internal.hashes[i],
            &cms->internal.buckets_indexes[i]);
    dchain_rejuvenate_index(cms->allocators[i],
                            cms->internal.buckets_indexes[i], now);
  }
}

int cms_fetch(struct CMS *cms) {
  int bucket_min_set = false;
  uint32_t *buckets_values[SKETCH_HASHES];
  uint32_t bucket_min = 0;

  for (int i = 0; i < SKETCH_HASHES; i++) {
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
  for (int i = 0; i < SKETCH_HASHES; i++) {
    int bucket_index = -1;
    int present =
        map_get(cms->clients, &cms->internal.hashes[i], &bucket_index);

    if (!present) {
      int allocated_client =
          dchain_allocate_new_index(cms->allocators[i], &bucket_index, now);

      if (!allocated_client) {
        // CMS size limit reached.
        return 0;
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

      return 1;
    } else {
      dchain_rejuvenate_index(cms->allocators[i], bucket_index, now);
      uint32_t *bucket;
      int offseted = bucket_index + cms->capacity * i;
      vector_borrow(cms->buckets, offseted, (void **)&bucket);
      (*bucket)++;
      vector_return(cms->buckets, offseted, bucket);
      return 1;
    }
  }

  return 1;
}

void cms_expire(struct CMS *cms, time_ns_t time) {
  int offset = 0;
  int index = -1;

  for (int i = 0; i < SKETCH_HASHES; i++) {
    offset = i * cms->capacity;

    while (dchain_expire_one_index(cms->allocators[i], &index, time)) {
      void *key;
      vector_borrow(cms->keys, index + offset, &key);
      map_erase(cms->clients, key, &key);
      vector_return(cms->keys, index + offset, key);
    }
  }
}

/**********************************************
 *
 *             CONTROLLER LOGIC
 *
 **********************************************/

bf_pkt *tx_pkt = nullptr;
bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;

bf_rt_target_t dev_tgt;
const bfrt::BfRtInfo *info;
std::shared_ptr<bfrt::BfRtSession> session;

struct eth_hdr_t {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t eth_type;
} __attribute__((packed));

struct ipv4_hdr_t {
  uint8_t ihl : 4;
  uint8_t version : 4;
  uint8_t ecn : 2;
  uint8_t dscp : 6;
  uint16_t tot_len;
  uint16_t id;
  uint16_t frag_off;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t check;
  uint32_t src_ip;
  uint32_t dst_ip;
} __attribute__((packed));

struct tcp_hdr_t {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq;
  uint32_t ack_seq;
  uint16_t res1;
  uint16_t window;
  uint16_t check;
  uint16_t urg_ptr;
} __attribute__((packed));

struct pkt_hdr_t {
  struct eth_hdr_t eth_hdr;
  struct ipv4_hdr_t ip_hdr;
  struct tcp_hdr_t tcp_hdr;
} __attribute__((packed));

void pretty_print_pkt(struct pkt_hdr_t *pkt_hdr) {
  printf("###[ Ethernet ]###\n");
  printf("  dst  %02x:%02x:%02x:%02x:%02x:%02x\n", pkt_hdr->eth_hdr.dst_mac[0],
         pkt_hdr->eth_hdr.dst_mac[1], pkt_hdr->eth_hdr.dst_mac[2],
         pkt_hdr->eth_hdr.dst_mac[3], pkt_hdr->eth_hdr.dst_mac[4],
         pkt_hdr->eth_hdr.dst_mac[5]);
  printf("  src  %02x:%02x:%02x:%02x:%02x:%02x\n", pkt_hdr->eth_hdr.src_mac[0],
         pkt_hdr->eth_hdr.src_mac[1], pkt_hdr->eth_hdr.src_mac[2],
         pkt_hdr->eth_hdr.src_mac[3], pkt_hdr->eth_hdr.src_mac[4],
         pkt_hdr->eth_hdr.src_mac[5]);
  printf("  type 0x%x\n", ntohs(pkt_hdr->eth_hdr.eth_type));

  printf("###[ IP ]###\n");
  printf("  ihl     %u\n", (pkt_hdr->ip_hdr.ihl & 0x0f));
  printf("  version %u\n", (pkt_hdr->ip_hdr.ihl & 0xf0) >> 4);
  printf("  tos     %u\n", pkt_hdr->ip_hdr.version);
  printf("  len     %u\n", ntohs(pkt_hdr->ip_hdr.tot_len));
  printf("  id      %u\n", ntohs(pkt_hdr->ip_hdr.id));
  printf("  off     %u\n", ntohs(pkt_hdr->ip_hdr.frag_off));
  printf("  ttl     %u\n", pkt_hdr->ip_hdr.ttl);
  printf("  proto   %u\n", pkt_hdr->ip_hdr.protocol);
  printf("  chksum  0x%x\n", ntohs(pkt_hdr->ip_hdr.check));
  printf("  src     %u.%u.%u.%u\n", (pkt_hdr->ip_hdr.src_ip >> 0) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 8) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 16) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 24) & 0xff);
  printf("  dst     %u.%u.%u.%u\n", (pkt_hdr->ip_hdr.dst_ip >> 0) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 8) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 16) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 24) & 0xff);

  printf("###[ UDP ]###\n");
  printf("  sport   %u\n", ntohs(pkt_hdr->tcp_hdr.src_port));
  printf("  dport   %u\n", ntohs(pkt_hdr->tcp_hdr.dst_port));
}

typedef int switch_int32_t;

char *get_env_var_value(const char *env_var) {
  auto env_var_value = getenv(env_var);

  if (!env_var_value) {
    std::cerr << env_var << " env var not found.\n";
    exit(1);
  }

  return env_var_value;
}

char *get_install_dir() { return get_env_var_value(ENV_VAR_SDE_INSTALL); }

bool nf_process(uint8_t *pkt, uint32_t packet_size);

void pcie_tx(bf_dev_id_t device, uint8_t *pkt, uint32_t packet_size) {
  if (bf_pkt_data_copy(tx_pkt, pkt, packet_size) != 0) {
    fprintf(stderr, "bf_pkt_data_copy failed: pkt_size=%d\n", packet_size);
    bf_pkt_free(device, tx_pkt);
    return;
  }

  if (bf_pkt_tx(device, tx_pkt, tx_ring, (void *)tx_pkt) != BF_SUCCESS) {
    fprintf(stderr, "bf_pkt_tx failed\n");
    bf_pkt_free(device, tx_pkt);
  }
}

bf_status_t txComplete(bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring,
                       uint64_t tx_cookie, uint32_t status) {
  return BF_SUCCESS;
}

bf_status_t pcie_rx(bf_dev_id_t device, bf_pkt *pkt, void *data,
                    bf_pkt_rx_ring_t rx_ring) {
  bf_pkt *orig_pkt = nullptr;
  char in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  char *pkt_buf = nullptr;
  char *bufp = nullptr;
  uint32_t packet_size = 0;
  switch_int32_t pkt_len = 0;

  // save a pointer to the packet
  orig_pkt = pkt;

  // assemble the received packet
  bufp = &in_packet[0];

  do {
    pkt_buf = (char *)bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);

    if ((packet_size + pkt_len) > SWITCH_PACKET_MAX_BUFFER_SIZE) {
      fprintf(stderr, "Packet too large to Transmit - SKipping\n");
      break;
    }

    memcpy(bufp, pkt_buf, pkt_len);
    bufp += pkt_len;
    packet_size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  auto atomic = true;
  auto hw_sync = true;

  session->beginTransaction(atomic);
  auto fwd = nf_process((uint8_t *)in_packet, packet_size);
  session->commitTransaction(hw_sync);

  if (fwd) {
    pcie_tx(device, (uint8_t *)in_packet, packet_size);
  }

  bf_pkt_free(device, orig_pkt);

  return BF_SUCCESS;
}

void register_pcie_pkt_ops() {
  if (bf_pkt_alloc(dev_tgt.dev_id, &tx_pkt, MTU, BF_DMA_CPU_PKT_TRANSMIT_0) !=
      0) {
    fprintf(stderr, "Failed to allocate packet buffer\n");
    exit(1);
  }

  // register callback for TX complete
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX;
       tx_ring++) {
    bf_pkt_tx_done_notif_register(dev_tgt.dev_id, txComplete,
                                  (bf_pkt_tx_ring_t)tx_ring);
  }

  // register callback for RX
  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX;
       rx_ring++) {
    auto status = bf_pkt_rx_register(dev_tgt.dev_id, pcie_rx,
                                     (bf_pkt_rx_ring_t)rx_ring, 0);
    if (status != BF_SUCCESS) {
      fprintf(stderr, "Failed to register pcie callback\n");
      exit(1);
    }
  }
}

void init_bf_switchd() {
  auto switchd_main_ctx =
      (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));

  /* Allocate memory to hold switchd configuration and state */
  if (switchd_main_ctx == NULL) {
    std::cerr << "ERROR: Failed to allocate memory for switchd context\n";
    exit(1);
  }

  char target_conf_file[100];
  sprintf(target_conf_file, "%s/share/p4/targets/tofino/%s.conf",
          get_install_dir(), PROGRAM_NAME);

  memset(switchd_main_ctx, 0, sizeof(bf_switchd_context_t));

  switchd_main_ctx->install_dir = get_install_dir();
  switchd_main_ctx->conf_file = const_cast<char *>(target_conf_file);
  switchd_main_ctx->skip_p4 = false;
  switchd_main_ctx->skip_port_add = false;
  switchd_main_ctx->running_in_background = false;
  switchd_main_ctx->dev_sts_thread = false;

  auto bf_status = bf_switchd_lib_init(switchd_main_ctx);

  if (bf_status != BF_SUCCESS) {
    exit(1);
  }
}

void setup_bf_session() {
  dev_tgt.dev_id = 0;
  dev_tgt.pipe_id = ALL_PIPES;

  // Get devMgr singleton instance
  auto &devMgr = bfrt::BfRtDevMgr::getInstance();

  // Get info object from dev_id and p4 program name
  auto bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, PROGRAM_NAME, &info);
  assert(bf_status == BF_SUCCESS);

  // Create a session object
  session = bfrt::BfRtSession::sessionCreate();
}

template <int key_size> bool key_eq(void *k1, void *k2) {
  auto _k1 = (uint8_t *)k1;
  auto _k2 = (uint8_t *)k2;

  for (auto i = 0; i < key_size; i++) {
    if (_k1[i] != _k2[i]) {
      return false;
    }
  }

  return true;
}

template <int key_size> unsigned key_hash(void *k) {
  auto _k = (uint8_t *)k;
  unsigned hash = 0;

  for (auto i = 0; i < key_size; i++) {
    hash = __builtin_ia32_crc32si(hash, _k[i]);
  }

  return hash;
}

class Table {
private:
  std::string table_name;
  const bfrt::BfRtTable *table;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

  std::vector<bf_rt_id_t> key_bytes_ids;
  bf_rt_id_t populate_action;
  bf_rt_id_t populate_action_value;

public:
  Table(const std::string &_table_name, int n_key_bytes)
      : table_name(_table_name), key_bytes_ids(n_key_bytes) {
    assert(info);
    assert(session);

    init_table();
    init_key();
    init_action();
    init_data();
  }

  void add(uint8_t *k, uint32_t value) {
    bf_status_t bf_status;

    bf_status = table->keyReset(key.get());
    assert(bf_status == BF_SUCCESS);
    bf_status = table->dataReset(populate_action, data.get());
    assert(bf_status == BF_SUCCESS);

    // Set value into the key object. Key type is "EXACT"
    for (auto byte = 0; byte < key_bytes_ids.size(); byte++) {
      printf("k[%d] = 0x%x\n", byte, k[byte]);
      auto key_byte = static_cast<uint64_t>(k[byte]);
      bf_status = key->setValue(key_bytes_ids[byte], key_byte);
      assert(bf_status == BF_SUCCESS);
    }

    // Set value into the data object
    printf("value = 0x%x\n", value);
    bf_status =
        data->setValue(populate_action_value, static_cast<uint64_t>(value));
    assert(bf_status == BF_SUCCESS);

    bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    assert(bf_status == BF_SUCCESS);
  }

  static std::unique_ptr<Table> build(const std::string &_table_name,
                                      int n_key_bytes) {
    return std::unique_ptr<Table>(new Table(_table_name, n_key_bytes));
  }

private:
  void init_table() {
    auto full_table_name = "Ingress." + table_name;
    auto bf_status = info->bfrtTableFromNameGet(full_table_name, &table);
    assert(bf_status == BF_SUCCESS);
  }

  void init_key() {
    for (auto byte = 0; byte < key_bytes_ids.size(); byte++) {
      std::stringstream key_label_builder;
      key_label_builder << "key_byte_" << byte;

      auto label = key_label_builder.str();
      auto &id = key_bytes_ids[byte];

      auto bf_status = table->keyFieldIdGet(label, &id);
      assert(bf_status == BF_SUCCESS);
    }

    auto bf_status = table->keyAllocate(&key);
    assert(bf_status == BF_SUCCESS);
    assert(key);
  }

  void init_action() {
    auto bf_status =
        table->actionIdGet("Ingress.map_populate", &populate_action);
    assert(bf_status == BF_SUCCESS);
  }

  void init_data() {
    auto bf_status =
        table->dataFieldIdGet("value", populate_action, &populate_action_value);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataAllocate(&data);
    assert(bf_status == BF_SUCCESS);
    assert(data);
  }
};

#define KEY_SIZE 2

struct state_t {
  std::unique_ptr<Table> dp_map;
  Map *map;

  state_t() {
    dp_map = Table::build("map", KEY_SIZE);
    map_allocate(&key_eq<KEY_SIZE>, &key_hash<KEY_SIZE>, 1000, &map);
  }
};

std::unique_ptr<state_t> state;

void nf_init() { state = std::unique_ptr<state_t>(new state_t()); }

bool nf_process(uint8_t *pkt, uint32_t size) {
  struct pkt_hdr_t *pkt_hdr = (struct pkt_hdr_t *)pkt;
  pretty_print_pkt(pkt_hdr);

  uint8_t key[KEY_SIZE];

  key[0] = (pkt_hdr->ip_hdr.src_ip >> 24) & 0xff;
  key[1] = (pkt_hdr->ip_hdr.src_ip >> 16) & 0xff;
  state->dp_map->add(key, 42);

  // key[0] = 0x01;
  // key[1] = 0x02;
  // state->dp_map->add(key, 42);

  return false;
}

int main(int argc, char **argv) {
  init_bf_switchd();
  setup_bf_session();
  nf_init();
  register_pcie_pkt_ops();

  std::cerr << "\nController is ready.\n";

  // main thread sleeps
  while (1) {
    sleep(60);
  }

  return 0;
}