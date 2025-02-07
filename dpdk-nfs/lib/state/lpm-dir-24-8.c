#include "lpm-dir-24-8.h"
#include "nf-parse.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <rte_byteorder.h>

#define LPM_INVALID 0xFFFF
#define LPM_PLEN_MAX 32
#define BYTE_SIZE 8

#define LPM_24_FLAG_MASK 0x8000     // == 0b1000 0000 0000 0000
#define LPM_24_MAX_ENTRIES 16777216 //= 2^24
#define LPM_24_VAL_MASK 0x7FFF
#define LPM_24_PLEN_MAX 24

#define LPM_LONG_OFFSET_MAX 256
#define LPM_LONG_FACTOR 256
#define LPM_LONG_MAX_ENTRIES 65536 //= 2^16

#define MAX_NEXT_HOP_VALUE 0x7FFF

struct rule {
  uint32_t ipv4;
  uint8_t prefixlen;
  uint16_t route;
};

struct LPM {
  uint16_t *lpm_24;
  uint16_t *lpm_long;
  uint16_t lpm_long_index;
};

void fill_invalid(uint16_t *t, uint32_t size) {
  for (uint32_t i = 0;; i++) {
    if (i == size) {
      break;
    }

    t[i] = LPM_INVALID;
  }
}

uint32_t build_mask_from_prefixlen(uint8_t prefixlen) {
  uint32_t ip_masks[33] = {0x00000000, 0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
                           0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000,
                           0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0,
                           0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

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
    uint32_t res[25] = {0x1000000, 0x800000, 0x400000, 0x200000, 0x100000, 0x80000, 0x40000, 0x20000, 0x10000,
                        0x8000,    0x4000,   0x2000,   0x1000,   0x800,    0x400,   0x200,   0x100,   0x80,
                        0x40,      0x20,     0x10,     0x8,      0x4,      0x2,     0x1};
    uint32_t v       = res[prefixlen];
    return v;
  } else {
    uint32_t res[8] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    uint32_t v      = res[prefixlen - 25];
    return v;
  }
}

bool lpm_24_entry_flag(uint16_t entry) { return (entry >> 15) == 1; }

uint16_t lpm_24_entry_set_flag(uint16_t entry) {
  uint16_t res = (uint16_t)(entry | LPM_24_FLAG_MASK);
  return res;
}

uint16_t lpm_long_extract_first_index(uint32_t data, uint8_t prefixlen, uint8_t base_index) {
  uint32_t mask        = build_mask_from_prefixlen(prefixlen);
  uint32_t masked_data = data & mask;
  uint8_t last_byte    = (uint8_t)(masked_data & 0xFF);
  uint16_t res         = (uint16_t)(base_index * (uint16_t)LPM_LONG_FACTOR + last_byte);

  return res;
}

int lpm_allocate(struct LPM **lpm_out) {
  struct LPM *lpm = (struct LPM *)malloc(sizeof(struct LPM));
  if (lpm == 0) {
    return 0;
  }

  uint16_t *lpm_24 = (uint16_t *)malloc(LPM_24_MAX_ENTRIES * sizeof(uint16_t));
  if (lpm_24 == 0) {
    free(lpm);
    return 0;
  }

  uint16_t *lpm_long = (uint16_t *)malloc(LPM_LONG_MAX_ENTRIES * sizeof(uint16_t));
  if (lpm_long == 0) {
    free(lpm_24);
    free(lpm);
    return 0;
  }

  // Set every element of the array to LPM_INVALID
  fill_invalid(lpm_24, LPM_24_MAX_ENTRIES);
  fill_invalid(lpm_long, LPM_LONG_MAX_ENTRIES);

  lpm->lpm_24                   = lpm_24;
  lpm->lpm_long                 = lpm_long;
  uint16_t lpm_long_first_index = 0;
  lpm->lpm_long_index           = lpm_long_first_index;

  *lpm_out = lpm;
  return 1;
}

void lpm_free(struct LPM *lpm) {
  free(lpm->lpm_24);
  free(lpm->lpm_long);
  free(lpm);
}

int lpm_lookup(struct LPM *lpm, uint32_t addr, uint16_t *value_out) {
  addr = rte_bswap32(addr);

  uint16_t *lpm_24   = lpm->lpm_24;
  uint16_t *lpm_long = lpm->lpm_long;

  // get index corresponding to key for lpm_24
  uint32_t index = lpm_24_extract_first_index(addr);

  uint16_t value = lpm_24[index];
  // Prove that the value retrieved by lookup_lpm_24 is the mapped value
  // retrieved by lpm_24[index]

  if (value != LPM_INVALID && lpm_24_entry_flag(value)) {
    // the value found in lpm_24 is a base index for an entry in lpm_long,
    // go look at the index corresponding to the key and this base index

    uint8_t extracted_index = (uint8_t)(value & 0xFF);
    uint16_t index_long     = lpm_long_extract_first_index(addr, 32, extracted_index);
    // Show that indexlong_from_ipv4 == compute_starting_index_long when
    // the rule has prefixlen == 32
    uint16_t value_long = lpm_long[index_long];

    if (value_long == LPM_INVALID) {
      return 0;
    } else {
      *value_out = value_long;
      return 1;
    }
  } else {
    if (value == LPM_INVALID) {
      return 0;
    } else {
      *value_out = value;
      return 1;
    }
  }
}

int lpm_update(struct LPM *lpm, uint32_t prefix, uint8_t prefixlen, uint16_t value) {
  prefix = rte_bswap32(prefix);

  uint16_t *lpm_24   = lpm->lpm_24;
  uint16_t *lpm_long = lpm->lpm_long;

  uint32_t mask      = build_mask_from_prefixlen(prefixlen);
  uint32_t masked_ip = prefix & mask;

  // If prefixlen is smaller than 24, simply store the value in lpm_24
  if (prefixlen < 25) {
    uint32_t first_index = lpm_24_extract_first_index(masked_ip);
    uint32_t rule_size   = compute_rule_size(prefixlen);
    uint32_t last_index  = first_index + rule_size;

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

    if (lpm_24_value == LPM_INVALID) {
      need_new_index = true;
    } else {
      need_new_index = !lpm_24_entry_flag(lpm_24_value);
    }

    if (need_new_index) {
      if (lpm->lpm_long_index >= LPM_LONG_OFFSET_MAX) {
        // No more available index for lpm_long
        return 0;
      } else {
        // generate next index and store it in lpm_24
        base_index          = (uint8_t)(lpm->lpm_long_index);
        new_long_index      = (uint16_t)(lpm->lpm_long_index + 1);
        lpm->lpm_long_index = new_long_index;

        uint16_t new_entry24 = lpm_24_entry_set_flag(base_index);
        lpm_24[lpm_24_index] = new_entry24;
      }
    } else {
      new_long_index = lpm->lpm_long_index;
      base_index     = (uint8_t)(lpm_24_value & 0x7FFF);
    }

    // The last byte in data is used as the starting offset for lpm_long
    // indexes
    uint32_t first_index = lpm_long_extract_first_index(prefix, prefixlen, base_index);

    uint32_t rule_size  = compute_rule_size(prefixlen);
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

static bool parse_ipv4addr(const char *str, uint32_t *addr) {
  uint8_t a, b, c, d;
  if (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) == 4) {
    *addr = ((uint32_t)a << 0) | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
    return true;
  }
  return false;
}

void lpm_from_file(struct LPM *lpm, const char *cfg_fname) {
  FILE *cfg_file = fopen(cfg_fname, "r");
  if (cfg_file == NULL) {
    rte_exit(EXIT_FAILURE, "Error opening the static config file: %s", cfg_fname);
  }

  char ipv4_addr_str[16];
  int subnet_size;
  uint16_t device;

  while (fscanf(cfg_file, "%15[^/]/%d %hu\n", ipv4_addr_str, &subnet_size, &device) == 3) {
    uint32_t ipv4_addr;
    if (!parse_ipv4addr(ipv4_addr_str, &ipv4_addr)) {
      rte_exit(EXIT_FAILURE, "Error parsing ipv4 address \"%s\" from cfg file", ipv4_addr_str);
    }

    lpm_update(lpm, ipv4_addr, subnet_size, device);
  }

  fclose(cfg_file);
}
