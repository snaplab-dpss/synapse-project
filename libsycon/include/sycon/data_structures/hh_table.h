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
#include "synapse_ds.h"

namespace sycon {

class HHTable : public SynapseDS {
private:
  constexpr const static u32 TOTAL_PROBES{50};
  constexpr const static u32 THRESHOLD{128};
  constexpr const static time_s_t RESET_TIMER{1};

  static const std::vector<u32> HASH_SALTS;

  std::vector<Table> tables;
  Register reg_cached_counters;
  std::vector<Register> count_min_sketch;
  Register reg_threshold;
  Digest digest;

  const u32 capacity;
  const bits_t key_size;

  const std::vector<buffer_t> hash_salts;
  const u32 hash_mask;
  const CRC32 crc32;

  std::unordered_map<buffer_t, u32, buffer_hash_t> key_to_index;
  std::unordered_map<u32, buffer_t> index_to_key;
  std::unordered_set<u32> free_indices;
  std::unordered_set<u32> used_indices;
  std::unordered_map<buffer_t, std::unordered_set<std::string>, buffer_hash_t> expirations_per_key;

public:
  HHTable(const std::string &name, const std::vector<std::string> &table_names, const std::string &reg_cached_counters_name,
          const std::vector<std::string> &count_min_sketch_reg_names, const std::string &reg_threshold_name, const std::string &digest_name,
          time_ms_t timeout);

  bool get(const buffer_t &k, u32 &v);

private:
  std::vector<u32> calculate_hashes(const buffer_t &key);
  u32 cms_get_min(const std::vector<u32> &hashes);

  bool is_index_allocated(u32 index) const;
  bool insert(const buffer_t &key);
  void replace(u32 index, const buffer_t &key);
  void probabilistic_replace(const buffer_t &key);
  void remove(const buffer_t &key);
  void clear_counters();

  static std::vector<Table> build_tables(const std::vector<std::string> &table_names);
  static std::vector<Register> build_count_min_sketch(const std::vector<std::string> &cms_reg_names);
  static u32 get_capacity(const std::vector<Table> &tables);
  static bits_t get_key_size(const std::vector<Table> &tables);
  static u32 build_hash_mask(const std::vector<Register> &count_min_sketch);
  static std::vector<buffer_t> build_hash_salts(const std::vector<Register> &count_min_sketch);

  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie);
  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie);
};

} // namespace sycon