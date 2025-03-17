#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../config.h"
#include "../constants.h"
#include "../primitives/table.h"
#include "../primitives/digest.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../field.h"
#include "../hash.h"

namespace sycon {

class HHTable {
private:
  constexpr const static u32 TOTAL_PROBES{5};
  constexpr const static u32 THRESHOLD{5};

  constexpr const static u32 HASH_SALT_0{0xfbc31fc7};
  constexpr const static u32 HASH_SALT_1{0x2681580b};
  constexpr const static u32 HASH_SALT_2{0x486d7e2f};

  std::vector<Table> tables;
  Register reg_cached_counters;
  Register reg_threshold;
  Digest digest;

  const u32 capacity;
  const bits_t key_size;

  const buffer_t hash_salt_0;
  const buffer_t hash_salt_1;
  const buffer_t hash_salt_2;
  const CRC32 crc32;

  std::unordered_map<buffer_t, u32, buffer_hash_t> cached_key_to_index;
  std::unordered_map<u32, buffer_t> cached_index_to_key;
  std::unordered_set<u32> free_indices;
  std::unordered_set<u32> used_indices;

public:
  HHTable(const std::vector<std::string> &table_names, const std::string &reg_cached_counters_name, const std::string &reg_threshold_name,
          const std::string &digest_name, time_ms_t timeout)
      : tables(build_tables(table_names)), reg_cached_counters(reg_cached_counters_name), reg_threshold(reg_threshold_name),
        digest(digest_name), capacity(get_capacity(tables)), key_size(get_key_size(tables)), hash_salt_0(build_hash_salt(HASH_SALT_0)),
        hash_salt_1(build_hash_salt(HASH_SALT_1)), hash_salt_2(build_hash_salt(HASH_SALT_2)), cached_key_to_index(capacity),
        cached_index_to_key(capacity), free_indices(capacity), used_indices(capacity) {
    reg_threshold.set(0, THRESHOLD);

    Table &chosen_expiration_table = tables.front();
    chosen_expiration_table.set_notify_mode(timeout, this, HHTable::expiration_callback, true);

    digest.register_callback(HHTable::digest_callback, this);

    for (u32 i = 0; i < capacity; i++) {
      free_indices.insert(i);
    }
  }

  bool insert(const buffer_t &key) {
    if (cached_key_to_index.find(key) != cached_key_to_index.end()) {
      return false;
    }

    if (free_indices.empty()) {
      return false;
    }

    u32 index = *free_indices.begin();
    free_indices.erase(index);
    used_indices.insert(index);

    cached_index_to_key.insert({index, key});
    cached_key_to_index.insert({key, index});

    return true;
  }

  void replace(const buffer_t &key, u32 index) {
    auto found_it = cached_index_to_key.find(index);
    assert(found_it != cached_index_to_key.end() && "Index not found in cache");

    found_it->second = key;
    cached_key_to_index.erase(key);
    cached_key_to_index.insert({key, index});
  }

  struct probed_key_t {
    buffer_t key;
    u32 counter;
  };

  std::array<probed_key_t, TOTAL_PROBES> probe_random_keys_counters() {
    std::array<probed_key_t, TOTAL_PROBES> probes;

    const size_t total_used_indices = used_indices.size();
    assert(total_used_indices > TOTAL_PROBES && "Not enough used indices");

    for (size_t i = 0; i < TOTAL_PROBES; i++) {
      const u32 index = rand() % total_used_indices;

      auto found_it = cached_index_to_key.find(index);
      assert(found_it != cached_index_to_key.end() && "Index not found in cache");

      const u32 counter = reg_cached_counters.get(index);
      probes[i]         = {found_it->second, counter};
    }

    return probes;
  }

  void clear_counters() const {}

private:
  static std::vector<Table> build_tables(const std::vector<std::string> &table_names) {
    assert(!table_names.empty() && "Table name must not be empty");

    std::vector<Table> tables;
    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
    }

    return tables;
  }

  static u32 get_capacity(const std::vector<Table> &tables) {
    u32 capacity = tables.back().get_capacity();
    for (const Table &table : tables) {
      assert(table.get_capacity() == capacity);
    }
    return capacity;
  }

  static bits_t get_key_size(const std::vector<Table> &tables) {
    bits_t key_size = 0;
    for (const table_field_t &field : tables.back().get_key_fields()) {
      key_size += field.size;
    }

    for (const Table &table : tables) {
      bits_t current_key_size = 0;
      for (const table_field_t &field : table.get_key_fields()) {
        current_key_size += field.size;
      }
      assert(current_key_size == key_size);
    }

    return key_size;
  }

  static buffer_t build_hash_salt(u32 hash_salt_value) {
    buffer_t hash_salt(4);
    hash_salt.set(0, 4, hash_salt_value);
    return hash_salt;
  }

  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_transaction();

    HHTable *map_table = reinterpret_cast<HHTable *>(cookie);
    assert(map_table && "Invalid cookie");

    const bfrt::BfRtTable *table;
    bf_status_t status = key->tableGet(&table);
    ASSERT_BF_STATUS(status);

    std::string table_name;
    status = table->tableNameGet(&table_name);
    ASSERT_BF_STATUS(status);

    buffer_t key_buffer;
    bool target_table_found = false;
    for (const Table &target_table : map_table->tables) {
      if (target_table.get_full_name() == table_name) {
        target_table_found = true;
        key_buffer         = target_table.get_key_value(key);
        break;
      }
    }

    if (!target_table_found) {
      ERROR("Target table %s not found", table_name.c_str());
    }

    cfg.end_transaction();
  }

  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
    // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
    HHTable *hh_table = const_cast<HHTable *>(reinterpret_cast<const HHTable *>(cookie));
    assert(hh_table && "Invalid cookie");

    DEBUG("[%s] Digest callback invoked", hh_table->digest.get_name().c_str());

    cfg.begin_transaction();

    for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
      buffer_t key = hh_table->digest.get_value(data_entry.get());
      LOG("Digest data: %s", key.to_string(true).c_str());

      u32 hash0 = hh_table->crc32.hash(key.append(hh_table->hash_salt_0));
      u32 hash1 = hh_table->crc32.hash(key.append(hh_table->hash_salt_1));
      u32 hash2 = hh_table->crc32.hash(key.append(hh_table->hash_salt_2));

      DEBUG("Hash0: 0x%08x, Hash1: 0x%08x, Hash2: 0x%08x", hash0, hash1, hash2);

      if (!hh_table->free_indices.empty()) {
        hh_table->insert(key);
      } else {
        assert(false && "TODO");
      }
    }

    hh_table->digest.notify_ack(learn_msg_hdl);
    cfg.end_transaction();

    return BF_SUCCESS;
  }
};

} // namespace sycon