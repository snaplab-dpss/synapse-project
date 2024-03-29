#include "lpm-dir-24-8.h"

struct rule {
  uint32_t ipv4;
  uint8_t prefixlen;
  uint16_t route;
};

struct lpm {
  uint16_t *lpm_24;
  uint16_t *lpm_long;
  uint16_t lpm_long_index;
};

void fill_invalid(uint16_t *t, uint32_t size) {
  for (uint32_t i = 0;; i++) {
    if (i == size) {
      break;
    }

    t[i] = INVALID;
  }
}

uint32_t build_mask_from_prefixlen(uint8_t prefixlen) {
  uint32_t ip_masks[33] = {
      0x00000000, 0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000,
      0xFC000000, 0xFE000000, 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
      0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000,
      0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
      0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8,
      0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

  return ip_masks[prefixlen];
}

// Extract the 24 MSB of an uint8_t array and returns them
uint32_t lpm_24_extract_first_index(uint32_t data) {
  uint32_t res = data >> BYTE_SIZE;
  return res;
}

// Computes how many entries the rule will take
uint32_t compute_rule_size(uint8_t prefixlen) {
  if (prefixlen < 25) {
    uint32_t res[25] = {0x1000000, 0x800000, 0x400000, 0x200000, 0x100000,
                        0x80000,   0x40000,  0x20000,  0x10000,  0x8000,
                        0x4000,    0x2000,   0x1000,   0x800,    0x400,
                        0x200,     0x100,    0x80,     0x40,     0x20,
                        0x10,      0x8,      0x4,      0x2,      0x1};
    uint32_t v = res[prefixlen];
    return v;
  } else {
    uint32_t res[8] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    uint32_t v = res[prefixlen - 25];
    return v;
  }
}

bool lpm_24_entry_flag(uint16_t entry) { return (entry >> 15) == 1; }

uint16_t lpm_24_entry_set_flag(uint16_t entry) {
  uint16_t res = (uint16_t)(entry | lpm_24_FLAG_MASK);
  return res;
}

uint16_t lpm_long_extract_first_index(uint32_t data, uint8_t prefixlen,
                                      uint8_t base_index) {
  uint32_t mask = build_mask_from_prefixlen(prefixlen);
  uint32_t masked_data = data & mask;
  uint8_t last_byte = (uint8_t)(masked_data & 0xFF);
  uint16_t res = (uint16_t)(base_index * (uint16_t)lpm_LONG_FACTOR + last_byte);

  return res;
}

int lpm_allocate(struct lpm **lpm_out) {
  struct lpm *_lpm = (struct lpm *)malloc(sizeof(struct lpm));
  if (_lpm == 0) {
    return 0;
  }

  uint16_t *lpm_24 = (uint16_t *)malloc(lpm_24_MAX_ENTRIES * sizeof(uint16_t));
  if (lpm_24 == 0) {
    free(_lpm);
    return 0;
  }

  uint16_t *lpm_long =
      (uint16_t *)malloc(lpm_LONG_MAX_ENTRIES * sizeof(uint16_t));
  if (lpm_long == 0) {
    free(lpm_24);
    free(_lpm);
    return 0;
  }

  // Set every element of the array to INVALID
  fill_invalid(lpm_24, lpm_24_MAX_ENTRIES);
  fill_invalid(lpm_long, lpm_LONG_MAX_ENTRIES);

  _lpm->lpm_24 = lpm_24;
  _lpm->lpm_long = lpm_long;
  uint16_t lpm_long_first_index = 0;
  _lpm->lpm_long_index = lpm_long_first_index;

  *lpm_out = _lpm;
  return 1;
}

void lpm_free(struct lpm *_lpm) {
  free(_lpm->lpm_24);
  free(_lpm->lpm_long);
  free(_lpm);
}

int lpm_lookup_elem(struct lpm *_lpm, uint32_t prefix) {
  uint16_t *lpm_24 = _lpm->lpm_24;
  uint16_t *lpm_long = _lpm->lpm_long;

  // get index corresponding to key for lpm_24
  uint32_t index = lpm_24_extract_first_index(prefix);

  uint16_t value = lpm_24[index];
  // Prove that the value retrieved by lookup_lpm_24 is the mapped value
  // retrieved by lpm_24[index]

  if (value != INVALID && lpm_24_entry_flag(value)) {
    // the value found in lpm_24 is a base index for an entry in lpm_long,
    // go look at the index corresponding to the key and this base index

    uint8_t extracted_index = (uint8_t)(value & 0xFF);
    uint16_t index_long =
        lpm_long_extract_first_index(prefix, 32, extracted_index);
    // Show that indexlong_from_ipv4 == compute_starting_index_long when
    // the rule has prefixlen == 32
    uint16_t value_long = lpm_long[index_long];

    if (value_long == INVALID) {
      return INVALID;
    } else {
      return value_long;
    }
  } else {
    if (value == INVALID) {
      return INVALID;
    } else {
      return value;
    }
  }
}

int lpm_update_elem(struct lpm *_lpm, uint32_t prefix, uint8_t prefixlen,
                    uint16_t value) {
  uint16_t *lpm_24 = _lpm->lpm_24;
  uint16_t *lpm_long = _lpm->lpm_long;

  uint32_t mask = build_mask_from_prefixlen(prefixlen);
  uint32_t masked_ip = prefix & mask;

  // If prefixlen is smaller than 24, simply store the value in lpm_24
  if (prefixlen < 25) {
    uint32_t first_index = lpm_24_extract_first_index(masked_ip);
    uint32_t rule_size = compute_rule_size(prefixlen);
    uint32_t last_index = first_index + rule_size;

    // fill all entries between [first index and last index[ with value
    for (uint32_t i = first_index;; i++) {
      if (i == last_index) {
        break;
      }

      lpm_24[i] = value;
    }
  } else {
    // If the prefixlen is not smaller than 24, we have to store the value
    // in lpm_long.

    // Check the lpm_24 entry corresponding to the key. If it already has a
    // flag set to 1, use the stored value as base index, otherwise get a
    // new index and store it in the lpm_24
    uint8_t base_index;
    uint32_t lpm_24_index = lpm_24_extract_first_index(prefix);
    uint16_t lpm_24_value = lpm_24[lpm_24_index];
    // Prove that the value retrieved by lookup_lpm_24 is the mapped value
    // retrieved by lpm_24[index]

    bool need_new_index;
    uint16_t new_long_index;

    if (lpm_24_value == INVALID) {
      need_new_index = true;
    } else {
      need_new_index = !lpm_24_entry_flag(lpm_24_value);
    }

    if (need_new_index) {
      if (_lpm->lpm_long_index >= lpm_LONG_OFFSET_MAX) {
        printf("No more available index for lpm_long!\n");
        fflush(stdout);
        return 0;

      } else {
        // generate next index and store it in lpm_24
        base_index = (uint8_t)(_lpm->lpm_long_index);
        new_long_index = (uint16_t)(_lpm->lpm_long_index + 1);
        _lpm->lpm_long_index = new_long_index;

        uint16_t new_entry24 = lpm_24_entry_set_flag(base_index);
        lpm_24[lpm_24_index] = new_entry24;
      }
    } else {
      new_long_index = _lpm->lpm_long_index;

      base_index = (uint8_t)(lpm_24_value & 0x7FFF);
    }

    // The last byte in data is used as the starting offset for lpm_long
    // indexes
    uint32_t first_index =
        lpm_long_extract_first_index(prefix, prefixlen, base_index);

    uint32_t rule_size = compute_rule_size(prefixlen);
    uint32_t last_index = first_index + rule_size;

    // Store value in lpm_long entries
    for (uint32_t i = first_index;; i++) {
      if (i == last_index) {
        break;
      }

      lpm_long[i] = value;
    }
  }

  return 1;
}
