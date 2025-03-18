#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../primitives/digest.h"
#include "../time.h"
#include "../field.h"
#include "../hash.h"

namespace sycon {

class HHTable {
private:
  constexpr const static u32 TOTAL_PROBES{50};
  constexpr const static u32 THRESHOLD{10'000};
  constexpr const static time_s_t RESET_TIMER{10};

  constexpr const static u32 HASH_SALT_0{0xfbc31fc7};
  constexpr const static u32 HASH_SALT_1{0x2681580b};
  constexpr const static u32 HASH_SALT_2{0x486d7e2f};

  std::vector<Table> tables;
  Register reg_cached_counters;
  std::vector<Register> bloom_filter;
  std::vector<Register> count_min_sketch;
  Register reg_threshold;
  Digest digest;

  const u32 capacity;
  const bits_t key_size;

  const std::vector<buffer_t> hash_salts;
  const u32 hash_mask;
  const CRC32 crc32;

  std::unordered_map<buffer_t, u32, buffer_hash_t> cached_key_to_index;
  std::unordered_map<u32, buffer_t> cached_index_to_key;
  std::unordered_set<u32> free_indices;
  std::unordered_set<u32> used_indices;

public:
  HHTable(const std::vector<std::string> &table_names, const std::string &reg_cached_counters_name,
          const std::vector<std::string> &bloom_filter_reg_names, const std::vector<std::string> &count_min_sketch_reg_names,
          const std::string &reg_threshold_name, const std::string &digest_name, time_ms_t timeout);

  bool insert(const buffer_t &key);
  void replace(u32 index, const buffer_t &key);
  void probabilistic_replace(const buffer_t &key);
  void remove(const buffer_t &key);
  void clear_counters();

private:
  std::vector<u32> calculate_hashes(const buffer_t &key);
  bool bloom_filter_query(const std::vector<u32> &hashes);
  u32 cms_get_min(const std::vector<u32> &hashes);

  static std::vector<Table> build_tables(const std::vector<std::string> &table_names);
  static std::vector<Register> build_bloom_filter(const std::vector<std::string> &bloom_filter_reg_names);
  static std::vector<Register> build_count_min_sketch(const std::vector<std::string> &cms_reg_names);
  static u32 get_capacity(const std::vector<Table> &tables);
  static bits_t get_key_size(const std::vector<Table> &tables);
  static u32 build_hash_mask(const std::vector<Register> &bloom_filter, const std::vector<Register> &count_min_sketch);
  static std::vector<buffer_t> build_hash_salts();

  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie);
  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie);
};

} // namespace sycon