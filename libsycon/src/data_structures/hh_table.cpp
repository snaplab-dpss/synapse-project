#include <array>
#include <optional>
#include <unordered_set>
#include <thread>

#include "../../include/sycon/data_structures/hh_table.h"
#include "../../include/sycon/config.h"

namespace sycon {

const std::vector<u32> HHTable::HASH_SALTS = {0xfbc31fc7, 0x2681580b, 0x486d7e2f, 0x1f3a2b4d, 0x7c5e9f8b, 0x3a2b4d1f,
                                              0x5e9f8b7c, 0x2b4d1f3a, 0x9f8b7c5e, 0xb4d1f3a2, 0x4d1f3a2b, 0x8b7c5e9f};

HHTable::HHTable(const std::string &_name, const std::vector<std::string> &table_names, const std::string &reg_cached_counters_name,
                 const std::vector<std::string> &count_min_sketch_reg_names, const std::vector<std::string> &bloom_filter_reg_names,
                 const std::string &reg_threshold_name, const std::string &digest_name, time_ms_t timeout)
    : SynapseDS(_name), tables(build_tables(table_names)), reg_cached_counters(reg_cached_counters_name),
      count_min_sketch(build_count_min_sketch(count_min_sketch_reg_names)), bloom_filter(build_bloom_filter(bloom_filter_reg_names)),
      reg_threshold(reg_threshold_name), digest(digest_name), capacity(get_capacity(tables)), key_size(get_key_size(tables)),
      hash_salts(build_hash_salts(count_min_sketch, bloom_filter)), hash_mask(build_hash_mask(bloom_filter, count_min_sketch)), crc32(),
      key_to_index(capacity), index_to_key(capacity), free_indices(capacity), used_indices(capacity) {
  assert(hash_salts.size() == bloom_filter.size() && "Number of salts must match the number of bloom filter registers");
  assert(hash_salts.size() == count_min_sketch.size() && "Number of salts must match the number of CMS registers");

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
      commit();
      cfg.commit_transaction();
    }
  }).detach();
}

bool HHTable::insert(const buffer_t &key, bool &duplicate_request_detected) {
  LOG_DEBUG("Inserting key %s", key.to_string(true).c_str());

  duplicate_request_detected = false;

  if (key_to_index.find(key) != key_to_index.end()) {
    duplicate_request_detected = true;
    return false;
  }

  if (free_indices.empty()) {
    return false;
  }

  const u32 index = *free_indices.begin();

  used_indexes_on_hold.insert(index);
  if (new_entries_on_hold.find(key) != new_entries_on_hold.end()) {
    new_entries_on_hold[key] = index;
  }

  buffer_t param(4);
  param.set(0, 4, index);

  for (Table &table : tables) {
    const std::vector<table_action_t> &actions = table.get_actions();
    assert(actions.size() == 1);
    const table_action_t &set_index_action = actions[0];

    if (!table.try_add_entry(key, set_index_action.name, {param})) {
      LOG_DEBUG("Failed to add entry to table %s", table.get_name().c_str());
      return false;
    }
  }

  free_indices.erase(index);
  used_indices.insert(index);

  index_to_key.insert({index, key});
  key_to_index.insert({key, index});

  reg_cached_counters.set(index, 0);

  return true;
}

void HHTable::replace(u32 index, const buffer_t &key, bool &duplicate_request_detected) {
  LOG_DEBUG("Replacing index %u with key %s", index, key.to_string(true).c_str());

  assert(key_to_index.find(key) == key_to_index.end() && "Key already exists in cache");

  duplicate_request_detected = false;

  auto found_it = index_to_key.find(index);
  assert(found_it != index_to_key.end() && "Index not found in cache");

  const buffer_t old_key = found_it->second;

  if (old_key == key) {
    duplicate_request_detected = true;
    return;
  }

  if (modified_entries_backup.find(key) != modified_entries_backup.end()) {
    modified_entries_backup[old_key] = index;
  }

  found_it->second = key;

  key_to_index.erase(old_key);
  key_to_index.insert({key, index});

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

void HHTable::remove(const buffer_t &key, bool &duplicate_request_detected) {
  LOG_DEBUG("Removing key %s", key.to_string(true).c_str());

  duplicate_request_detected = false;

  auto found_it = key_to_index.find(key);
  if (found_it == key_to_index.end()) {
    duplicate_request_detected = true;
    return;
  }

  const u32 index = found_it->second;

  free_indexes_on_hold.insert(index);
  if (modified_entries_backup.find(key) != modified_entries_backup.end()) {
    modified_entries_backup[key] = index;
  }

  key_to_index.erase(key);
  index_to_key.erase(index);

  free_indices.insert(index);
  used_indices.erase(index);

  for (Table &table : tables) {
    table.del_entry(key);
  }
}

void HHTable::probabilistic_replace(const buffer_t &key, bool &duplicate_request_detected) {
  const std::vector<u32> hashes   = calculate_hashes(key);
  const u32 key_counter           = cms_get_min(hashes);
  const size_t total_used_indices = used_indices.size();

  LOG_DEBUG("Key counter: %u", key_counter);

  for (size_t i = 0; i < TOTAL_PROBES; i++) {
    const u32 probe_index = rand() % total_used_indices;

    auto found_it = index_to_key.find(probe_index);
    assert(found_it != index_to_key.end() && "Index not found in cache");

    const buffer_t &probe_key = found_it->second;
    const u32 probe_counter   = reg_cached_counters.get_max(probe_index);

    LOG_DEBUG("Probe index %u counter: %u", probe_index, probe_counter);

    if (key_counter > probe_counter) {
      replace(key_to_index.at(probe_key), key, duplicate_request_detected);
      return;
    }
  }
}

void HHTable::clear_counters() {
  LOG_DEBUG("Cleaning HHTable counters...");

  for (Register &reg : count_min_sketch) {
    reg.overwrite_all_entries(0);
  }

  for (Register &reg : bloom_filter) {
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

  if (capacity > 1024) {
    // Actually, we usually only get around 90% of usage from the dataplane tables.
    // Higher than that and we start getting collisions, and errors trying to insert new entries.
    capacity *= 0.9;
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

std::vector<buffer_t> HHTable::build_hash_salts(const std::vector<Register> &count_min_sketch, const std::vector<Register> &bloom_filter) {
  assert(bloom_filter.size() == count_min_sketch.size() && "Bloom filter and CMS must have the same number of registers");

  const size_t num_hashes = bloom_filter.size();
  assert(HASH_SALTS.size() >= num_hashes && "Not enough hash salts defined");

  std::vector<buffer_t> hash_salts;
  for (size_t i = 0; i < num_hashes; i++) {
    buffer_t hash_salt(4);
    hash_salt.set(0, 4, HASH_SALTS[i]);
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

  bool duplicate_request_detected;
  hh_table->remove(key_buffer, duplicate_request_detected);

  if (duplicate_request_detected) {
    hh_table->rollback();
    cfg.abort_transaction();
  } else {
    hh_table->commit();
    cfg.commit_transaction();
  }
}

bf_status_t HHTable::digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
  // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
  HHTable *hh_table = const_cast<HHTable *>(reinterpret_cast<const HHTable *>(cookie));
  assert(hh_table && "Invalid cookie");

  cfg.begin_transaction();

  bool duplicate_request_detected;

  for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
    const buffer_t key = hh_table->digest.get_value(data_entry.get());

    LOG_DEBUG("[%s] Digest callback invoked (data=%s)", hh_table->digest.get_name().c_str(), key.to_string(true).c_str());

    if (!hh_table->free_indices.empty()) {
      hh_table->insert(key, duplicate_request_detected);
    } else {
      hh_table->probabilistic_replace(key, duplicate_request_detected);
    }

    if (duplicate_request_detected) {
      break;
    }
  }

  hh_table->digest.notify_ack(learn_msg_hdl);

  if (duplicate_request_detected) {
    hh_table->rollback();
    cfg.abort_transaction();
  } else {
    hh_table->commit();
    cfg.commit_transaction();
  }

  return BF_SUCCESS;
}

void HHTable::rollback() {
  if (new_entries_on_hold.empty() && modified_entries_backup.empty() && free_indexes_on_hold.empty() && used_indexes_on_hold.empty()) {
    return;
  }

  LOG_DEBUG("[%s] Aborted tx: rolling back state", name.c_str());

  for (const auto &[key, index] : new_entries_on_hold) {
    key_to_index.erase(key);
    index_to_key.erase(index);
  }

  for (const auto &[key, index] : modified_entries_backup) {
    key_to_index[key]   = index;
    index_to_key[index] = key;
  }

  for (u32 index : free_indexes_on_hold) {
    free_indices.erase(index);
    used_indices.insert(index);
  }

  for (u32 index : used_indexes_on_hold) {
    used_indices.erase(index);
    free_indices.insert(index);
  }

  new_entries_on_hold.clear();
  modified_entries_backup.clear();
  free_indexes_on_hold.clear();
  used_indexes_on_hold.clear();
}

void HHTable::commit() {
  new_entries_on_hold.clear();
  modified_entries_backup.clear();
  free_indexes_on_hold.clear();
  used_indexes_on_hold.clear();
}

} // namespace sycon