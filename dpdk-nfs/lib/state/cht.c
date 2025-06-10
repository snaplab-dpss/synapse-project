#include "cht.h"

#include "lib/util/compute.h"
#include "lib/util/hash.h"

#include <assert.h>
#include <stdlib.h>

#define HASH_SALT_0 0x9b78350f
#define HASH_SALT_1 0x32ed75e2

struct CHT {
  int *table;
  uint32_t height;
  uint32_t key_size;
  uint32_t backends;
};

static void clear(struct CHT *cht) {
  if (cht == NULL || cht->table == NULL) {
    return;
  }
  memset(cht->table, -1, sizeof(int) * cht->height);
}

int cht_allocate(uint32_t height, uint32_t key_size, struct CHT **cht_out) {
  if (height == 0 || key_size == 0) {
    return 0;
  }

  if (!is_prime(height)) {
    // Ensure that the height is a prime number
    return 0;
  }

  struct CHT *cht = (struct CHT *)malloc(sizeof(struct CHT));
  if (cht == NULL) {
    return 0;
  }

  cht->height   = height;
  cht->key_size = key_size;
  cht->backends = 0;

  cht->table = (int *)malloc(sizeof(int) * height);
  if (cht->table == NULL) {
    free(cht);
    return 0;
  }

  clear(cht);

  *cht_out = cht;
  return 1;
}

static uint32_t loop(uint32_t k, uint32_t height) {
  uint32_t g = k % height;
  return g;
}

static int populate(struct CHT *cht) {
  if (cht->backends == 0) {
    return 0;
  }

  // Generate the permutations of 0..(cht_height - 1) for each backend
  uint32_t *permutations = (uint32_t *)malloc(sizeof(uint32_t) * cht->height * cht->backends);
  if (permutations == 0) {
    return 0;
  }

  for (uint32_t i = 0; i < cht->backends; ++i) {
    struct hash_input_t {
      uint32_t backend_id;
      uint32_t salt;
    };

    struct hash_input_t hash_input_0 = {.backend_id = i, .salt = HASH_SALT_0};
    struct hash_input_t hash_input_1 = {.backend_id = i, .salt = HASH_SALT_1};

    uint32_t h0 = hash_obj(&hash_input_0, sizeof(hash_input_0));
    uint32_t h1 = hash_obj(&hash_input_1, sizeof(hash_input_1));

    uint32_t offset = loop(h0, cht->height);
    uint32_t skip   = loop(h1, cht->height - 1) + 1;

    for (uint32_t j = 0; j < cht->height; ++j) {
      permutations[i * cht->height + j] = loop(offset + skip * j, cht->height);
    }
  }

  int *next = (int *)calloc(sizeof(int), cht->height);
  if (next == 0) {
    free(permutations);
    return 0;
  }

  uint32_t n = 0;
  while (1) {
    for (uint32_t i = 0; i < cht->backends; ++i) {
      uint32_t p;
      do {
        p = permutations[i * cht->height + next[i]];
        next[i]++;
      } while (cht->table[p] >= 0);

      cht->table[p] = (int)i;
      next[i]++;
      n++;

      if (n == cht->height) {
        free(next);
        free(permutations);
        return 1;
      }
    }
  }

  free(next);
  free(permutations);

  return 1;
}

int cht_add_backend(struct CHT *cht, int backend) {
  if (cht->backends >= cht->height) {
    return 0; // No space for more backends
  }

  if (backend < 0 || backend >= cht->height) {
    return 0; // Invalid backend index
  }

  cht->backends++;

  clear(cht);

  if (!populate(cht)) {
    // Failed to fill the CHT, rollback.
    // This should really not happen unless there is no more available memory.
    cht->backends--;
    return 0;
  }

  return 1;
}

int cht_remove_backend(struct CHT *cht, int backend) {
  bool found = false;
  for (uint32_t i = 0; i < cht->height; ++i) {
    if (cht->table[i] == backend) {
      cht->table[i] = -1; // Mark as removed
      found         = true;
    }
  }

  if (found) {
    cht->backends--;
    return 1;
  }

  return 0;
}

int cht_find_backend(struct CHT *cht, void *key, int *chosen_backend_out) {
  uint32_t hash       = hash_obj(key, cht->key_size);
  uint32_t i          = loop(hash, cht->height);
  *chosen_backend_out = cht->table[i];

  if (*chosen_backend_out == -1) {
    return 0;
  }

  return 1;
}
