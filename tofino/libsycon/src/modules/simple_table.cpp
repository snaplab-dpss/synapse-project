#include "../../include/sycon/modules/simple_table.h"

#include <optional>
#include <unordered_set>

#include "../../include/sycon/log.h"
#include "../constants.h"

namespace sycon {

static void log_entry_op(const Table *table, const table_key_t &key,
                         const table_data_t &data, const std::string &op) {
  assert(table);

  const auto &key_fields = table->get_key_fields();
  const auto &data_fields = table->get_data_fields();
  const auto &name = table->get_name();

  DEBUG();
  DEBUG("*********************************************");

  assert(key.size > 0);
  assert(key.size == key_fields.size());

  DEBUG("Time: %lu", time(0));
  DEBUG("[%s] Op: %s", name.c_str(), op.c_str());

  DEBUG("  Key:");
  for (auto i = 0u; i < key.size; i++) {
    auto key_field = key_fields[i];
    auto key_value = key.values[i];
    DEBUG("     %s : 0x%02lx", key_field.name.c_str(), key_value);
  }

  if (data.size) {
    assert(data.size <= data_fields.size());

    DEBUG("  Data:");
    for (auto i = 0u; i < data.size; i++) {
      auto data_field = data_fields[i];
      auto data_value = data.values[i];
      DEBUG("     %s : 0x%02lx", data_field.name.c_str(), data_value);
    }
  }

  DEBUG("*********************************************\n");
}

fields_values_t::fields_values_t() : size(0), values(nullptr) {}

fields_values_t::fields_values_t(uint32_t _size)
    : size(_size), values(new field_value_t[_size]) {}

fields_values_t::fields_values_t(fields_values_t &&other)
    : size(other.size), values(std::move(other.values)) {
  other.size = 0;
  other.values = nullptr;
}

fields_values_t::fields_values_t(const fields_values_t &other)
    : size(other.size), values(new field_value_t[other.size]) {
  for (auto i = 0u; i < size; i++) {
    values[i] = other.values[i];
  }
}

fields_values_t::~fields_values_t() {
  if (values) delete values;
  size = 0;
}

bool fields_values_t::operator==(const fields_values_t &other) {
  if (size != other.size) {
    return false;
  }

  for (uint32_t i = 0; i < other.size; i++) {
    if (values[i] != other.values[i]) {
      return false;
    }
  }

  return true;
}

std::size_t fields_values_t::hash::operator()(const fields_values_t &k) const {
  int hash = 0;

  for (auto i = 0u; i < k.size; i++) {
    hash ^= std::hash<int>()(k.values[i]);
  }

  return hash;
}

std::ostream &operator<<(std::ostream &os,
                         const fields_values_t &fields_values) {
  os << "{";
  for (uint32_t i = 0; i < fields_values.size; i++) {
    if (i > 0) os << ",";
    os << "0x" << std::hex << fields_values.values[i] << std::dec;
  }
  os << "}";
  return os;
}

table_entry_t::table_entry_t(const table_key_t &_key) : key(_key) {}

table_entry_t::table_entry_t(const table_key_t &_key, const table_data_t &_data)
    : key(_key), data(_data) {}

bool operator==(const table_entry_t &lhs, const table_entry_t &rhs) {
  return lhs.key == rhs.key;
}

std::size_t table_entry_t::hash::operator()(const table_entry_t &entry) const {
  return fields_values_t::hash()(entry.key);
}

void entries_t::clear() { entries.clear(); }

void entries_t::set(const entries_t &other) { entries = other.entries; }

void entries_t::assert_entries() const {
  for (const auto &entry : entries) {
    assert(entry.key.size > 0);
  }
}

void entries_t::add(const table_entry_t &entry) {
  assert(!contains(entry.key));
  entries.insert(entry);
  assert_entries();
}

void entries_t::add(const table_key_t &key, const table_data_t &data) {
  assert(!contains(key));
  entries.emplace(key, data);
  assert_entries();
}

void entries_t::add(const table_key_t &key) {
  assert(!contains(key));
  entries.emplace(key);
  assert_entries();
}

void entries_t::erase(const table_key_t &key) {
  assert(contains(key));
  table_entry_t entry(key);
  entries.erase(entry);
}

bool entries_t::contains(const table_key_t &key) const {
  table_entry_t entry(key);
  return entries.find(entry) != entries.end();
}

SimpleTable::SimpleTable(const std::string &_control_name,
                         const std::string &_table_name)
    : Table(_control_name, _table_name) {
  init_key();
  init_action("populate", &populate_action_id);
  init_data_with_action(populate_action_id);
}

SimpleTable::SimpleTable(const std::string &_control_name,
                         const std::string &_table_name, time_ms_t _timeout,
                         const bfrt::BfRtIdleTmoExpiryCb &_callback)
    : SimpleTable(_control_name, _table_name) {
  cookie.table = this;
  timeout = _timeout;
  callback = _callback;

  assert(*timeout >= TOFINO_MIN_EXPIRATION_TIME);

  set_notify_mode(*timeout, &cookie, *callback, false);
  enable_expirations();
}

void SimpleTable::enable_expirations() {
  assert(timeout && callback);
  set_notify_mode(*timeout, &cookie, *callback, true);
}

void SimpleTable::disable_expirations() {
  assert(timeout && callback);
  set_notify_mode(*timeout, &cookie, *callback, false);
}

void SimpleTable::set(const bytes_t &key_bytes,
                      std::vector<bytes_t> params_bytes) {
  if (timeout) {
    params_bytes.emplace_back(4, *timeout);
  }

  auto key = bytes_to_fields_values(key_bytes, key_fields);
  auto data = bytes_to_fields_values(params_bytes, data_fields);

  if (!added_entries.contains(key)) {
    add(key, data);
    added_entries.add(key, data);
  }
}

void SimpleTable::set(const bytes_t &key_bytes) {
  if (timeout) {
    set(key_bytes, {});
    return;
  }

  auto key = bytes_to_fields_values(key_bytes, key_fields);

  if (!added_entries.contains(key)) {
    add(key);
    added_entries.add(key);
  }
}

void SimpleTable::del(const bytes_t &key_bytes) {
  auto key = bytes_to_fields_values(key_bytes, key_fields);
  del(key);
}

void SimpleTable::del(const bfrt::BfRtTableKey *_key) {
  auto key = key_to_fields_values(_key);

  if (added_entries.contains(key)) {
    del(key);
    added_entries.erase(key);
  }
}

std::unique_ptr<SimpleTable> SimpleTable::build(
    const std::string &_control_name, const std::string &_table_name) {
  return std::unique_ptr<SimpleTable>(
      new SimpleTable(_control_name, _table_name));
}

std::unique_ptr<SimpleTable> SimpleTable::build(
    const std::string &_control_name, const std::string &_table_name,
    time_ms_t _timeout, const bfrt::BfRtIdleTmoExpiryCb &_callback) {
  return std::unique_ptr<SimpleTable>(
      new SimpleTable(_control_name, _table_name, _timeout, _callback));
}

void SimpleTable::key_setup() {
  auto bf_status = table->keyReset(key.get());
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::key_setup(const table_key_t &key_field_values) {
  key_setup();

  for (auto i = 0u; i < key_fields.size(); i++) {
    auto key_field_value = key_field_values.values[i];
    auto key_field = key_fields[i];
    auto bf_status = key->setValue(key_field.id, key_field_value);
    ASSERT_BF_STATUS(bf_status)
  }
}

void SimpleTable::data_setup() {
  auto bf_status = table->dataReset(populate_action_id, data.get());
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::data_setup(const table_data_t &param_field_values) {
  data_setup();

  assert(param_field_values.size <= data_fields.size());

  for (auto i = 0u; i < param_field_values.size; i++) {
    auto param_field_value = param_field_values.values[i];
    auto param_field = data_fields[i];
    auto bf_status = data->setValue(param_field.id, param_field_value);
    ASSERT_BF_STATUS(bf_status)
  }
}

table_data_t SimpleTable::get(const table_key_t &key_fields_values) {
  log_entry_op(this, key_fields_values, table_data_t(), "GET");

  key_setup(key_fields_values);
  data_setup();

  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status =
      table->tableEntryGet(*session, dev_tgt, flags, *key, data.get());
  ASSERT_BF_STATUS(bf_status)

  auto data_fields = data_to_fields_values(data.get());
  return data_fields;
}

void SimpleTable::add(const table_key_t &key_fields_values,
                      const table_data_t &param_fields_values) {
  log_entry_op(this, key_fields_values, param_fields_values, "ADD");

  key_setup(key_fields_values);
  data_setup(param_fields_values);

  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::add(const table_key_t &key_fields_values) {
  log_entry_op(this, key_fields_values, table_data_t(), "ADD");

  key_setup(key_fields_values);
  data_setup();

  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::mod(const table_key_t &key_fields_values,
                      const table_data_t &param_fields_values) {
  log_entry_op(this, key_fields_values, table_data_t(), "MOD");

  key_setup(key_fields_values);
  data_setup(param_fields_values);

  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status = table->tableEntryMod(*session, dev_tgt, flags, *key, *data);
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::del(const table_key_t &key_fields_values) {
  log_entry_op(this, key_fields_values, table_data_t(), "DEL");

  key_setup(key_fields_values);

  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status = table->tableEntryDel(*session, dev_tgt, flags, *key);
  ASSERT_BF_STATUS(bf_status)
}

void SimpleTable::clear() {
  uint64_t flags = 0;
  BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

  auto bf_status = table->tableClear(*session, dev_tgt, flags);
  ASSERT_BF_STATUS(bf_status)

  DEBUG();
  DEBUG("*********************************************");
  DEBUG("Time: %lu\n", time(0));
  DEBUG("[%s] Clear\n", name.c_str());
  DEBUG("*********************************************");
}

table_key_t SimpleTable::key_to_fields_values(const bfrt::BfRtTableKey *key) {
  auto fields_values = table_key_t(key_fields.size());

  for (auto i = 0u; i < key_fields.size(); i++) {
    auto field = key_fields[i];
    auto bf_status = key->getValue(field.id, &fields_values.values[i]);
    ASSERT_BF_STATUS(bf_status)
  }

  return fields_values;
}

table_data_t SimpleTable::data_to_fields_values(
    const bfrt::BfRtTableData *data) {
  auto fields_values = table_data_t(data_fields.size());

  for (auto i = 0u; i < data_fields.size(); i++) {
    auto field = data_fields[i];
    auto bf_status = data->getValue(field.id, &fields_values.values[i]);
    ASSERT_BF_STATUS(bf_status)
  }

  return fields_values;
}

fields_values_t SimpleTable::bytes_to_fields_values(
    const bytes_t &values, const std::vector<table_field_t> &fields) {
  auto fields_values = fields_values_t(fields.size());
  auto starting_byte = 0;

  for (auto kf_i = 0u; kf_i < fields.size(); kf_i++) {
    auto kf = fields[kf_i];
    auto bytes = kf.size / 8;

    assert(values.size >= starting_byte + bytes);

    fields_values.values[kf_i] = 0;

    for (auto i = 0u; i < bytes; i++) {
      auto byte = values[starting_byte + i];

      fields_values.values[kf_i] = (fields_values.values[kf_i] << 8) | byte;
    }

    starting_byte += bytes;
  }

  return fields_values;
}

// one to one relation
fields_values_t SimpleTable::bytes_to_fields_values(
    const std::vector<bytes_t> &values,
    const std::vector<table_field_t> &fields) {
  auto fields_values = fields_values_t(values.size());

  assert(fields.size() >= values.size());

  for (auto i = 0u; i < values.size(); i++) {
    auto value = values[i];
    auto field = fields[i];
    auto bytes = field.size / 8;

    assert(value.size == bytes);

    fields_values.values[i] = 0;

    for (auto j = 0u; j < bytes; j++) {
      fields_values.values[i] = (fields_values.values[i] << 8) | value[j];
    }
  }

  return fields_values;
}

}  // namespace sycon