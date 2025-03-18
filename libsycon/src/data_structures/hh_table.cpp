#include <array>
#include <optional>
#include <unordered_set>
#include <thread>

#include "../../include/sycon/data_structures/hh_table.h"
#include "../../include/sycon/config.h"

namespace sycon {

HHTable::HHTable(const std::vector<std::string> &table_names, const std::string &reg_cached_counters_name,
                 const std::vector<std::string> &bloom_filter_reg_names, const std::vector<std::string> &count_min_sketch_reg_names,
                 const std::string &reg_threshold_name, const std::string &digest_name, time_ms_t timeout)
    : tables(build_tables(table_names)), reg_cached_counters(reg_cached_counters_name),
      bloom_filter(build_bloom_filter(bloom_filter_reg_names)), count_min_sketch(build_count_min_sketch(count_min_sketch_reg_names)),
      reg_threshold(reg_threshold_name), digest(digest_name), capacity(get_capacity(tables)), key_size(get_key_size(tables)),
      hash_salts(build_hash_salts()), hash_mask(build_hash_mask(bloom_filter, count_min_sketch)), crc32(), cached_key_to_index(capacity),
      cached_index_to_key(capacity), free_indices(capacity), used_indices(capacity) {
  reg_threshold.set(0, THRESHOLD);

  Table &chosen_expiration_table = tables.front();
  chosen_expiration_table.set_notify_mode(timeout, this, HHTable::expiration_callback, true);

  digest.register_callback(HHTable::digest_callback, this);

  for (u32 i = 0; i < capacity; i++) {
    free_indices.insert(i);
  }

  std::thread([this]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(RESET_TIMER));
      cfg.begin_transaction();
      clear_counters();
      cfg.end_transaction();
    }
  }).detach();
}

bool HHTable::insert(const buffer_t &key) {
  LOG_DEBUG("Inserting key %s", key.to_string(true).c_str());

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

  buffer_t param(4);
  param.set(0, 4, index);

  for (Table &table : tables) {
    const std::vector<table_action_t> &actions = table.get_actions();
    assert(actions.size() == 1);
    const table_action_t &set_index_action = actions[0];

    table.add_entry(key, set_index_action.name, {param});
  }

  reg_cached_counters.set(index, 0);

  return true;
}

void HHTable::replace(u32 index, const buffer_t &key) {
  LOG_DEBUG("Replacing index %u with key %s", index, key.to_string(true).c_str());

  assert(cached_key_to_index.find(key) == cached_key_to_index.end() && "Key already exists in cache");

  auto found_it = cached_index_to_key.find(index);
  assert(found_it != cached_index_to_key.end() && "Index not found in cache");

  const buffer_t old_key = found_it->second;
  found_it->second       = key;

  cached_key_to_index.erase(old_key);
  cached_key_to_index.insert({key, index});

  buffer_t param(4);
  param.set(0, 4, index);

  for (Table &table : tables) {
    const std::vector<table_action_t> &actions = table.get_actions();
    assert(actions.size() == 1);
    const table_action_t &set_index_action = actions[0];

    table.del_entry(old_key);
    table.add_entry(key, set_index_action.name, {param});
  }

  reg_cached_counters.set(index, 0);
}

void HHTable::remove(const buffer_t &key) {
  LOG_DEBUG("Removing key %s", key.to_string(true).c_str());

  auto found_it = cached_key_to_index.find(key);
  if (found_it == cached_key_to_index.end()) {
    return;
  }

  const u32 index = found_it->second;
  cached_key_to_index.erase(key);
  cached_index_to_key.erase(index);

  free_indices.insert(index);
  used_indices.erase(index);

  for (Table &table : tables) {
    table.del_entry(key);
  }
}

void HHTable::probabilistic_replace(const buffer_t &key) {
  const std::vector<u32> hashes   = calculate_hashes(key);
  const u32 key_counter           = cms_get_min(hashes);
  const size_t total_used_indices = used_indices.size();

  LOG_DEBUG("Key counter: %u", key_counter);

  for (size_t i = 0; i < TOTAL_PROBES; i++) {
    const u32 probe_index = rand() % total_used_indices;

    auto found_it = cached_index_to_key.find(probe_index);
    assert(found_it != cached_index_to_key.end() && "Index not found in cache");

    const buffer_t &probe_key = found_it->second;
    const u32 probe_counter   = reg_cached_counters.get_max(probe_index);

    LOG_DEBUG("Probe index %u counter: %u", probe_index, probe_counter);

    if (key_counter > probe_counter) {
      replace(cached_key_to_index.at(probe_key), key);
      return;
    }
  }
}

void HHTable::clear_counters() {
  LOG_DEBUG("Cleaning HHTable counters...");

  for (Register &reg : bloom_filter) {
    reg.overwrite_all_entries(0);
  }

  for (Register &reg : count_min_sketch) {
    reg.overwrite_all_entries(0);
  }

  reg_cached_counters.overwrite_all_entries(0);
}

bool HHTable::bloom_filter_query(const std::vector<u32> &hashes) {
  assert(hashes.size() == bloom_filter.size());

  for (size_t i = 0; i < hashes.size(); i++) {
    if (!bloom_filter[i].get_max(hashes[i])) {
      return false;
    }
  }

  return true;
}

u32 HHTable::cms_get_min(const std::vector<u32> &hashes) {
  assert(hashes.size() == count_min_sketch.size());

  u32 min = std::numeric_limits<u32>::max();
  for (size_t i = 0; i < hashes.size(); i++) {
    min = std::min(min, count_min_sketch[i].get_max(hashes[i]));
  }

  return min;
}

std::vector<u32> HHTable::calculate_hashes(const buffer_t &key) {
  std::vector<u32> hashes;
  for (const buffer_t &hash_salt : hash_salts) {
    const buffer_t hash_input = key.append(hash_salt);
    const u32 hash            = crc32.hash(hash_input) & hash_mask;
    hashes.push_back(hash);
  }
  return hashes;
}

std::vector<Table> HHTable::build_tables(const std::vector<std::string> &table_names) {
  assert(!table_names.empty() && "Table name must not be empty");

  std::vector<Table> tables;
  for (const std::string &table_name : table_names) {
    tables.emplace_back(table_name);
  }

  return tables;
}

std::vector<Register> HHTable::build_bloom_filter(const std::vector<std::string> &bloom_filter_reg_names) {
  assert(!bloom_filter_reg_names.empty() && "Bloom filter register names must not be empty");

  std::vector<Register> bloom_filter;
  for (const std::string &name : bloom_filter_reg_names) {
    bloom_filter.emplace_back(name);
  }

  return bloom_filter;
}

std::vector<Register> HHTable::build_count_min_sketch(const std::vector<std::string> &cms_reg_names) {
  assert(!cms_reg_names.empty() && "CMS register names must not be empty");

  std::vector<Register> cms;
  for (const std::string &name : cms_reg_names) {
    cms.emplace_back(name);
  }

  return cms;
}

u32 HHTable::get_capacity(const std::vector<Table> &tables) {
  u32 capacity = tables.back().get_capacity();
  for (const Table &table : tables) {
    assert(table.get_capacity() == capacity);
  }
  return capacity;
}

bits_t HHTable::get_key_size(const std::vector<Table> &tables) {
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

u32 HHTable::build_hash_mask(const std::vector<Register> &bloom_filter, const std::vector<Register> &count_min_sketch) {
  assert(!bloom_filter.empty());
  assert(bloom_filter.size() == count_min_sketch.size());

  auto hash_size_from_capacity = [](size_t capacity) {
    assert((capacity & (capacity - 1)) == 0 && "Hash size must be a power of 2");
    bits_t hash_size = 0;
    while (capacity >>= 1) {
      hash_size++;
    }
    return hash_size;
  };

  const size_t capacity  = bloom_filter[0].get_capacity();
  const bits_t hash_size = hash_size_from_capacity(capacity);
  const u32 hash_mask    = (1ULL << hash_size) - 1;

  for (const Register &reg : bloom_filter) {
    assert(reg.get_capacity() == capacity);
  }

  for (const Register &reg : count_min_sketch) {
    assert(reg.get_capacity() == capacity);
  }

  return hash_mask;
}

std::vector<buffer_t> HHTable::build_hash_salts() {
  const std::vector<u32> salts{HASH_SALT_0, HASH_SALT_1, HASH_SALT_2};

  std::vector<buffer_t> hash_salts;
  for (u32 salt : salts) {
    buffer_t hash_salt(4);
    hash_salt.set(0, 4, salt);
    hash_salts.push_back(hash_salt);
  }

  return hash_salts;
}

void HHTable::expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
  cfg.begin_transaction();

  HHTable *hh_table = reinterpret_cast<HHTable *>(cookie);
  assert(hh_table && "Invalid cookie");

  const bfrt::BfRtTable *table;
  bf_status_t status = key->tableGet(&table);
  ASSERT_BF_STATUS(status);

  std::string table_name;
  status = table->tableNameGet(&table_name);
  ASSERT_BF_STATUS(status);

  buffer_t key_buffer;
  bool target_table_found = false;
  for (const Table &target_table : hh_table->tables) {
    if (target_table.get_full_name() == table_name) {
      target_table_found = true;
      key_buffer         = target_table.get_key_value(key);
      break;
    }
  }

  if (!target_table_found) {
    ERROR("Target table %s not found", table_name.c_str());
  }

  hh_table->remove(key_buffer);

  cfg.end_transaction();
}

bf_status_t HHTable::digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
  // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
  HHTable *hh_table = const_cast<HHTable *>(reinterpret_cast<const HHTable *>(cookie));
  assert(hh_table && "Invalid cookie");

  cfg.begin_transaction();

  for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
    const buffer_t key = hh_table->digest.get_value(data_entry.get());

    LOG_DEBUG("[%s] Digest callback invoked (data=%s)", hh_table->digest.get_name().c_str(), key.to_string(true).c_str());

    if (!hh_table->free_indices.empty()) {
      hh_table->insert(key);
    } else {
      hh_table->probabilistic_replace(key);
    }
  }

  hh_table->digest.notify_ack(learn_msg_hdl);
  cfg.end_transaction();

  return BF_SUCCESS;
}

} // namespace sycon