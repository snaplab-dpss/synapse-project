#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unistd.h>
#include <filesystem>

namespace netcache {

struct kv_t {
  uint32_t store_size;
  uint32_t initial_entries;
};

struct key_cntr_t {
  uint32_t reg_size;
  uint16_t reset_timer;
};

struct cm_t {
  uint32_t reg_size;
  uint16_t reset_timer;
};

struct bloom_t {
  uint32_t reg_size;
  uint16_t reset_timer;
};

struct conf_t {
  kv_t kv;
  key_cntr_t key_cntr;
  cm_t cm;
  bloom_t bloom;
};

conf_t parse_conf_file(const std::filesystem::path &conf_file_path);

} // namespace netcache
