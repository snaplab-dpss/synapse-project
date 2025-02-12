#include "../../include/sycon/primitives/table.hpp"

#include <sstream>

#include "../../include/sycon/config.hpp"
#include "../../include/sycon/constants.hpp"
#include "../../include/sycon/log.hpp"
#include "../../include/sycon/util.hpp"

namespace sycon {

std::string Table::append_control(const std::string &control, const std::string &name) {
  if (control.size()) {
    return control + "." + name;
  }

  return name;
}

Table::Table(const std::string &_control, const std::string &_name)
    : dev_tgt(cfg.dev_tgt), info(cfg.info), session(cfg.session), control(_control), name(_name), table(nullptr), time_aware(false) {
  auto full_name        = append_control(control, name);
  bf_status_t bf_status = info->bfrtTableFromNameGet(full_name, &table);
  ASSERT_BF_STATUS(bf_status)
}

void Table::set_session(const std::shared_ptr<bfrt::BfRtSession> &_session) { session = _session; }

void Table::init_key() {
  if (!key) {
    // Allocate key and data once, and use reset across different uses
    bf_status_t bf_status = table->keyAllocate(&key);
    ASSERT_BF_STATUS(bf_status)
    assert(key);
  }

  std::vector<bf_rt_id_t> key_fields_ids;

  bf_status_t bf_status = table->keyFieldIdListGet(&key_fields_ids);
  ASSERT_BF_STATUS(bf_status)

  for (auto id : key_fields_ids) {
    std::string name;
    bit_len_t size;

    bf_status = table->keyFieldNameGet(id, &name);
    ASSERT_BF_STATUS(bf_status)

    bf_status = table->keyFieldSizeGet(id, &size);
    ASSERT_BF_STATUS(bf_status)

    key_fields.push_back({name, id, size});
  }

  DEBUG("Keys:");
  for (auto k : key_fields) {
    DEBUG("  %s (%lu bits)", k.name.c_str(), k.size);
  }
}

void Table::init_key(const std::unordered_map<std::string, bf_rt_id_t *> &fields) {
  if (!key) {
    // Allocate key and data once, and use reset across different uses
    bf_status_t bf_status = table->keyAllocate(&key);
    ASSERT_BF_STATUS(bf_status)
    assert(key);
  }

  for (const auto &field : fields) {
    bf_status_t bf_status = table->keyFieldIdGet(field.first, field.second);
    ASSERT_BF_STATUS(bf_status)
  }
}

void Table::init_data(const std::unordered_map<std::string, bf_rt_id_t *> &fields) {
  if (!data) {
    bf_status_t bf_status = table->dataAllocate(&data);
    ASSERT_BF_STATUS(bf_status)
    assert(data);
  }

  for (const auto &field : fields) {
    bf_status_t bf_status = table->dataFieldIdGet(field.first, field.second);
    ASSERT_BF_STATUS(bf_status)
  }

  table->dataFieldIdGet(DATA_FIELD_NAME_ENTRY_TTL, &entry_ttl_data_id);
}

void Table::init_data_with_action(bf_rt_id_t action_id) {
  if (!data) {
    bf_status_t bf_status = table->dataAllocate(&data);
    ASSERT_BF_STATUS(bf_status)
    assert(data);
  }

  std::vector<bf_rt_id_t> data_fields_ids;

  bf_status_t bf_status = table->dataFieldIdListGet(action_id, &data_fields_ids);
  ASSERT_BF_STATUS(bf_status)

  DEBUG("Data:");

  for (auto id : data_fields_ids) {
    std::string name;
    bit_len_t size;

    bf_status = table->dataFieldNameGet(id, action_id, &name);
    ASSERT_BF_STATUS(bf_status)

    bf_status = table->dataFieldSizeGet(id, action_id, &size);
    ASSERT_BF_STATUS(bf_status)

    // Meta entries start with $ (e.g. $ENTRY_TTL).
    // We don't store those entries on data_fields.
    if (name[0] != '$') {
      data_fields.push_back({name, id, size});
    } else if (name == DATA_FIELD_NAME_ENTRY_TTL) {
      entry_ttl_data_id = id;
    }

    DEBUG("  %s (%lu bits)", name.c_str(), size);
  }
}

void Table::init_data_with_action(const std::string &name, bf_rt_id_t action_id, bf_rt_id_t *field_id) {
  if (!data) {
    bf_status_t bf_status = table->dataAllocate(&data);
    ASSERT_BF_STATUS(bf_status)
    assert(data);
  }

  bf_status_t bf_status = table->dataFieldIdGet(name, action_id, field_id);
  ASSERT_BF_STATUS(bf_status)
}

void Table::init_data_with_actions(const std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>> &fields) {
  for (const auto &field : fields) {
    init_data_with_action(field.first, field.second.first, field.second.second);
  }
}

void Table::init_action(const std::string &action_name, bf_rt_id_t *action_id) {
  auto full_action_name = append_control(control, action_name);
  bf_status_t bf_status = table->actionIdGet(full_action_name, action_id);
  ASSERT_BF_STATUS(bf_status)
}

void Table::init_action(bf_rt_id_t *action_id) {
  std::vector<bf_rt_id_t> action_ids;
  bf_status_t bf_status = table->actionIdListGet(&action_ids);
  ASSERT_BF_STATUS(bf_status)

  // Even tables with only a single explicit action, the NoAction action will
  // still be there by default.
  assert(action_ids.size() == 2);

  std::string action_name;
  bf_status = table->actionNameGet(action_ids[0], &action_name);
  ASSERT_BF_STATUS(bf_status)

  *action_id = (action_name != TOFINO_NO_ACTION_NAME) ? action_ids[0] : action_ids[1];
}

void Table::init_actions(const std::unordered_map<std::string, bf_rt_id_t *> &actions) {
  for (const auto &action : actions) {
    init_action(action.first, action.second);
  }
}

void Table::set_notify_mode(time_ms_t timeout, void *cookie, const bfrt::BfRtIdleTmoExpiryCb &callback, bool enable) {
  DEBUG("Set timeouts state for table %s: %d", name.c_str(), enable);

  assert(timeout >= TOFINO_MIN_EXPIRATION_TIME);

  std::unique_ptr<bfrt::BfRtTableAttributes> attr;

  bf_status_t bf_status =
      table->attributeAllocate(bfrt::TableAttributesType::IDLE_TABLE_RUNTIME, bfrt::TableAttributesIdleTableMode::NOTIFY_MODE, &attr);
  ASSERT_BF_STATUS(bf_status)

  u32 min_ttl            = timeout;
  u32 max_ttl            = timeout;
  u32 ttl_query_interval = timeout;

  assert(ttl_query_interval <= min_ttl);

  bf_status = attr->idleTableNotifyModeSet(enable, callback, ttl_query_interval, max_ttl, min_ttl, cookie);
  ASSERT_BF_STATUS(bf_status)

  u32 flags = 0;
  bf_status = table->tableAttributesSet(*session, dev_tgt, flags, *attr.get());
  ASSERT_BF_STATUS(bf_status)

  // Even if they are inactive, the table is still time aware.
  time_aware = true;
}

std::string Table::get_name() const { return name; }

size_t Table::get_size() const {
  size_t size;
  bf_status_t bf_status = table->tableSizeGet(*session, dev_tgt, &size);
  ASSERT_BF_STATUS(bf_status)
  return size;
}

size_t Table::get_usage() const {
  u32 usage;
  bf_status_t bf_status = table->tableUsageGet(*session, dev_tgt, bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW, &usage);
  ASSERT_BF_STATUS(bf_status)
  return usage;
}

const std::vector<table_field_t> &Table::get_key_fields() const { return key_fields; }

const std::vector<table_field_t> &Table::get_data_fields() const { return data_fields; }

void Table::dump_data_fields() const {
  std::stringstream ss;
  dump_data_fields(ss);
  LOG("%s", ss.str().c_str());
}

void Table::dump_data_fields(std::ostream &os) const {
  std::vector<bf_rt_id_t> ids;

  bf_status_t bf_status = table->dataFieldIdListGet(&ids);
  ASSERT_BF_STATUS(bf_status);

  for (auto id : ids) {
    std::string name;
    table->dataFieldNameGet(id, &name);
    os << "data " << name << "\n";
  }
}

static void dump_key(std::ostream &os, const bfrt::BfRtTable *table, bfrt::BfRtTableKey *key) {
  std::vector<bf_rt_id_t> key_fields_ids;

  bf_status_t bf_status = table->keyFieldIdListGet(&key_fields_ids);
  ASSERT_BF_STATUS(bf_status);

  for (auto key_field_id : key_fields_ids) {
    std::string key_field_name;
    bfrt::DataType key_field_data_type;
    bfrt::KeyFieldType key_field_type;

    bf_status = table->keyFieldNameGet(key_field_id, &key_field_name);
    ASSERT_BF_STATUS(bf_status);

    os << "  (key)    " << key_field_name << " = ";

    bf_status = table->keyFieldDataTypeGet(key_field_id, &key_field_data_type);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->keyFieldTypeGet(key_field_id, &key_field_type);
    ASSERT_BF_STATUS(bf_status);

    switch (key_field_data_type) {
    case bfrt::DataType::BOOL: {
      ERROR("Bool type handling not implemented for key field data types")
    } break;
    case bfrt::DataType::UINT64: {
      u64 key_field_value;

      switch (key_field_type) {
      case bfrt::KeyFieldType::EXACT: {
        bf_status = key->getValue(key_field_id, &key_field_value);
        ASSERT_BF_STATUS(bf_status);
        os << key_field_value;
      } break;
      case bfrt::KeyFieldType::TERNARY: {
        u64 key_field_mask;
        bf_status = key->getValueandMask(key_field_id, &key_field_value, &key_field_mask);
        ASSERT_BF_STATUS(bf_status);
        os << key_field_value;
        os << " (mask=" << key_field_mask << ")";
      } break;
      default: {
        ERROR("Only Exact and Ternary key types are implemented for uint64 "
              "data type")
      }
      }
    } break;
    case bfrt::DataType::FLOAT: {
      ERROR("Float type handling not implemented for key field data types")
    } break;
    case bfrt::DataType::STRING: {
      std::string key_field_value;
      bf_status = key->getValue(key_field_id, &key_field_value);
      ASSERT_BF_STATUS(bf_status);
      os << key_field_value;

      switch (key_field_type) {
      case bfrt::KeyFieldType::EXACT: {
        bf_status = key->getValue(key_field_id, &key_field_value);
        ASSERT_BF_STATUS(bf_status);
        os << key_field_value;
      } break;
      default: {
        ERROR("Only Exact key type is implemented for String data type")
      }
      }
    } break;
    case bfrt::DataType::INT_ARR: {
      ERROR("Int array type handling not implemented for key field data types")
    } break;
    case bfrt::DataType::BOOL_ARR: {
      ERROR("Bool array type handling not implemented for key field data types")
    } break;
    case bfrt::DataType::BYTE_STREAM: {
      size_t size;
      std::vector<u8> value;

      bf_status = table->keyFieldSizeGet(key_field_id, &size);
      ASSERT_BF_STATUS(bf_status);

      if (size % 8 == 0) {
        size /= 8;
      } else {
        size = (size / 8) + 1;
      }
      value.resize(size);

      switch (key_field_type) {
      case bfrt::KeyFieldType::EXACT: {
        bf_status = key->getValue(key_field_id, size, value.data());
        ASSERT_BF_STATUS(bf_status);

        for (auto v : value) {
          os << " 0x";
          os << std::setw(2) << std::setfill('0') << std::hex << (int)v;
        }
      } break;
      case bfrt::KeyFieldType::TERNARY: {
        std::vector<u8> mask(size);
        bf_status = key->getValueandMask(key_field_id, size, value.data(), mask.data());
        ASSERT_BF_STATUS(bf_status)
        for (auto i = 0u; i < value.size(); i++) {
          auto v = value[i];
          auto m = mask[i];
          os << " 0x";
          os << std::setw(2) << std::setfill('0') << std::hex << (int)v;
          os << " (mask=0x";
          os << std::setw(2) << std::setfill('0') << std::hex << (int)m;
          os << ")";
        }
      } break;
      default: {
        ERROR("Only Exact and Ternary key types are implemented for byte "
              "stream data type")
      }
      }
    } break;
    case bfrt::DataType::CONTAINER: {
      ERROR("Container type handling not implemented for data types")
    } break;
    case bfrt::DataType::STRING_ARR: {
      ERROR("String array type handling not implemented for key field data "
            "types")
    } break;
    default: {
      ERROR("Unexpected key field data type")
      assert(false && "Unexpected key field data type");
    }
    }
    os << "\n";
  }
}

static void dump_value(std::ostream &os, const std::vector<u64> &value) {
  os << "[";
  for (auto entry : value) {
    os << " " << entry;
  }
  os << " ]";
}

static void dump_value(std::ostream &os, const std::vector<u8> &value) {
  for (auto v : value) {
    os << " 0x";
    os << std::setw(2) << std::setfill('0') << std::hex << (int)v;
  }

  u64 v = 0;
  for (auto byte : value) {
    v = (v << 8) | byte;
  }
  os << " (" << v << ")";
}

static void dump_value(std::ostream &os, const std::vector<bool> &value) {
  os << "[";
  for (auto entry : value) {
    if (entry) {
      os << " true";
    } else {
      os << " false";
    }
  }
  os << " ]";
}

static void dump_value(std::ostream &os, const std::vector<std::string> &value) {
  os << "[";
  for (auto entry : value) {
    os << " " << entry;
  }
  os << " ]";
}

static void dump_data(std::ostream &os, const bfrt::BfRtTable *table, bfrt::BfRtTableData *data, bool time_aware) {
  bf_rt_id_t action_id;
  std::vector<bf_rt_id_t> data_fields_ids;
  bf_status_t bf_status;
  bool has_action = table->actionIdApplicable();

  if (has_action) {
    data->actionIdGet(&action_id);

    std::string action_name;
    bf_status = table->actionNameGet(action_id, &action_name);
    ASSERT_BF_STATUS(bf_status);

    os << "  (action) " << action_name << "\n";

    bf_status = table->dataFieldIdListGet(action_id, &data_fields_ids);
    ASSERT_BF_STATUS(bf_status);
  } else {
    bf_status = table->dataFieldIdListGet(&data_fields_ids);
    ASSERT_BF_STATUS(bf_status);
  }

  for (auto data_field_id : data_fields_ids) {
    std::string data_field_name;
    bfrt::DataType data_field_data_type;

    if (has_action) {
      bf_status = table->dataFieldNameGet(data_field_id, action_id, &data_field_name);
    } else {
      bf_status = table->dataFieldNameGet(data_field_id, &data_field_name);
    }

    ASSERT_BF_STATUS(bf_status);

    // Apparently, the ENTRY HIT STATE data is only available for tables
    // configured with POLL MODE, not NOTIFY MODE.
    if (data_field_name == DATA_FIELD_NAME_ENTRY_HIT_STATE && time_aware) {
      continue;
    }

    os << "  (data)   " << data_field_name << " ";

    if (has_action) {
      bf_status = table->dataFieldDataTypeGet(data_field_id, action_id, &data_field_data_type);
    } else {
      bf_status = table->dataFieldDataTypeGet(data_field_id, &data_field_data_type);
    }

    ASSERT_BF_STATUS(bf_status);

    switch (data_field_data_type) {
    case bfrt::DataType::BOOL: {
      bool data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      os << data_field_value;
    } break;
    case bfrt::DataType::UINT64: {
      u64 data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      os << data_field_value;
    } break;
    case bfrt::DataType::FLOAT: {
      float data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      os << data_field_value;
    } break;
    case bfrt::DataType::STRING: {
      std::string data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      os << data_field_value;
    } break;
    case bfrt::DataType::INT_ARR: {
      std::vector<u64> data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      dump_value(os, data_field_value);
    } break;
    case bfrt::DataType::BOOL_ARR: {
      std::vector<bool> data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      dump_value(os, data_field_value);
    } break;
    case bfrt::DataType::BYTE_STREAM: {
      size_t size;
      std::vector<u8> value;

      if (has_action) {
        bf_status = table->dataFieldSizeGet(data_field_id, action_id, &size);
        ASSERT_BF_STATUS(bf_status);
      } else {
        bf_status = table->dataFieldSizeGet(data_field_id, &size);
        ASSERT_BF_STATUS(bf_status);
      }

      size = (size % 8 == 0) ? (size / 8) : ((size / 8) + 1);
      value.resize(size);

      bf_status = data->getValue(data_field_id, size, value.data());
      ASSERT_BF_STATUS(bf_status);
      dump_value(os, value);
    } break;
    case bfrt::DataType::CONTAINER: {
      ERROR("Container type handling not implemented for data types");
    } break;
    case bfrt::DataType::STRING_ARR: {
      std::vector<std::string> data_field_value;
      bf_status = data->getValue(data_field_id, &data_field_value);
      ASSERT_BF_STATUS(bf_status);
      dump_value(os, data_field_value);
    } break;
    default: {
      ERROR("Not implemented");
    }
    }
    os << "\n";
  }
}

static void dump_entry(std::ostream &os, const bfrt::BfRtTable *table, bfrt::BfRtTableKey *key, bfrt::BfRtTableData *data,
                       bool time_aware) {
  std::vector<bf_rt_id_t> key_fields_ids;

  bf_status_t bf_status = table->keyFieldIdListGet(&key_fields_ids);
  ASSERT_BF_STATUS(bf_status);

  dump_key(os, table, key);
  dump_data(os, table, data, time_aware);
  os << "\n";
}

struct kd_t {
  std::vector<std::unique_ptr<bfrt::BfRtTableKey>> keys;
  std::vector<std::unique_ptr<bfrt::BfRtTableData>> datas;
  bfrt::BfRtTable::keyDataPairs pairs;

  kd_t(const bfrt::BfRtTable *table, u32 n) : keys(n), datas(n), pairs(n) {
    for (auto i = 0u; i < n; i++) {
      bf_status_t bf_status = table->keyAllocate(&keys[i]);
      ASSERT_BF_STATUS(bf_status);

      bf_status = table->dataAllocate(&datas[i]);
      ASSERT_BF_STATUS(bf_status);

      pairs[i] = std::pair<bfrt::BfRtTableKey *, bfrt::BfRtTableData *>(keys[i].get(), datas[i].get());
    }
  }
};

void Table::dump() const {
  std::stringstream ss;
  dump(ss);
  LOG("%s", ss.str().c_str());
}

void Table::dump(std::ostream &os) const {
  bf_status_t bf_status;
  u32 total_entries;
  u32 processed_entries;
  auto flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;

  os << "\n";
  os << "================================================\n";
  os << "  Table name: " << name << "\n";

  total_entries = get_usage();

  os << "\n";

  processed_entries = 0;

  bf_status = table->keyReset(key.get());
  ASSERT_BF_STATUS(bf_status);

  bf_status = table->dataReset(data.get());
  ASSERT_BF_STATUS(bf_status);

  while (processed_entries != total_entries) {
    if (processed_entries == 0) {
      bf_status = table->tableEntryGetFirst(*session, dev_tgt, flag, key.get(), data.get());
      ASSERT_BF_STATUS(bf_status);
      processed_entries++;

      dump_entry(os, table, key.get(), data.get(), time_aware);
    } else {
      u32 to_request = total_entries - processed_entries;
      u32 num_returned;

      kd_t kd(table, to_request);

      bf_status = table->tableEntryGetNext_n(*session, dev_tgt, *key, to_request, flag, &kd.pairs, &num_returned);
      ASSERT_BF_STATUS(bf_status);

      assert(num_returned == to_request);
      processed_entries += num_returned;

      for (auto i = 0u; i < num_returned; i++) {
        dump_entry(os, table, kd.keys[i].get(), kd.datas[i].get(), time_aware);
      }
    }
  }

  os << "  Entries: " << std::dec << total_entries << "\n";

  os << "================================================\n";
}

} // namespace sycon