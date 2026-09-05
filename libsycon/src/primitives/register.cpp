#include "../../include/sycon/primitives/register.h"

#include "../../include/sycon/log.h"

namespace sycon {

Register::Register(const std::string &_name) : MetaTable(_name), hi_id(0), paired(false) {
  init_fields();

  bf_status_t bf_status = table->dataFieldSizeGet(value_id, &value_size);
  ASSERT_BF_STATUS(bf_status);
}

Register::Register(const Register &other)
    : MetaTable(other), index_id(other.index_id), value_id(other.value_id), hi_id(other.hi_id), paired(other.paired),
      value_size(other.value_size), pipes(other.pipes) {
  init_fields();
}

void Register::init_fields() {
  init_key({{"$REGISTER_INDEX", &index_id}});

  // Plain register: a single data field "<name>.f1". Struct register (pair): "<name>.lo"/"<name>.hi".
  bf_status_t bf_status = table->dataFieldIdGet(name + ".f1", &value_id);
  if (bf_status == BF_SUCCESS) {
    paired = false;
    return;
  }

  bf_status = table->dataFieldIdGet(name + ".lo", &value_id);
  if (bf_status != BF_SUCCESS) {
    std::vector<bf_rt_id_t> field_ids;
    table->dataFieldIdListGet(&field_ids);
    std::string fields;
    for (bf_rt_id_t field_id : field_ids) {
      std::string field_name;
      table->dataFieldNameGet(field_id, &field_name);
      fields += " " + field_name;
    }
    ERROR("Register %s: neither %s.f1 nor %s.lo found (data fields:%s)", name.c_str(), name.c_str(), name.c_str(), fields.c_str());
  }

  init_data({{name + ".hi", &hi_id}});
  paired = true;
}

std::vector<u32> Register::get_per_pipe(u32 i) {
  key_setup(i);
  data_reset();

  bfrt::BfRtTable::BfRtTableGetFlag flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  bf_status_t bf_status                  = table->tableEntryGet(*session, dev_tgt, *key, flag, data.get());
  ASSERT_BF_STATUS(bf_status);

  std::vector<u64> values_per_pipe;
  bf_status = data->getValue(value_id, &values_per_pipe);
  ASSERT_BF_STATUS(bf_status);
  assert(!values_per_pipe.empty());

  std::vector<u32> values_per_pipe_32b;
  for (u64 value : values_per_pipe) {
    values_per_pipe_32b.push_back(static_cast<u32>(value));
  }

  return values_per_pipe_32b;
}

u32 Register::get_max(u32 i) {
  std::vector<u32> values_per_pipe = get_per_pipe(i);
  return *std::max_element(values_per_pipe.begin(), values_per_pipe.end());
}

u32 Register::get_min(u32 i) {
  std::vector<u32> values_per_pipe = get_per_pipe(i);
  return *std::min_element(values_per_pipe.begin(), values_per_pipe.end());
}

void Register::set(u32 i, u32 value) {
  key_setup(i);
  data_setup(value);

  bf_status_t bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status);
}

void Register::set(u32 i, u32 value, u16 pipe_id) {
  key_setup(i);
  data_setup(value);

  bf_rt_target_t target = dev_tgt;
  target.pipe_id        = pipe_id;

  bf_status_t bf_status = table->tableEntryMod(*session, target, *key, *data);
  ASSERT_BF_STATUS(bf_status);
}

void Register::overwrite_all_entries(u32 value) {
  data_setup(value);

  for (size_t i = 0; i < capacity; i++) {
    key_setup(i);

    bf_status_t bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }
}

void Register::key_setup(u32 i) {
  table->keyReset(key.get());

  bf_status_t bf_status = key->setValue(index_id, static_cast<u64>(i));
  ASSERT_BF_STATUS(bf_status);
}

void Register::data_setup(u32 value) {
  data_reset();

  bf_status_t bf_status = data->setValue(value_id, static_cast<uint64_t>(value));
  ASSERT_BF_STATUS(bf_status);
}

void Register::data_reset() { table->dataReset(data.get()); }

bits_t Register::get_value_size() const { return value_size; }

void Register::dump(std::ostream &os) const {
  bf_status_t bf_status;
  bfrt::BfRtTable::BfRtTableGetFlag flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

  os << "\n";
  os << "================================================\n";
  os << "  Register name: " << name << "\n";

  os << "\n";

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

  bf_status = table->keyAllocate(&key);
  ASSERT_BF_STATUS(bf_status);

  bf_status = table->dataAllocate(&data);
  ASSERT_BF_STATUS(bf_status);

  for (size_t i = 0; i < capacity; i++) {
    bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataReset(data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(index_id, static_cast<uint64_t>(i));
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->tableEntryGet(*session, dev_tgt, *key, flag, data.get());
    ASSERT_BF_STATUS(bf_status);

    os << "  [" << std::dec << i << "] = ";

    std::vector<u64> values_per_pipe;
    bf_status = data->getValue(value_id, &values_per_pipe);
    ASSERT_BF_STATUS(bf_status);
    for (size_t i = 0; i < values_per_pipe.size(); i++) {
      os << values_per_pipe[i] << " ";
    }

    if (paired) {
      std::vector<u64> hi_per_pipe;
      bf_status = data->getValue(hi_id, &hi_per_pipe);
      ASSERT_BF_STATUS(bf_status);
      os << "(hi: ";
      for (size_t i = 0; i < hi_per_pipe.size(); i++) {
        os << hi_per_pipe[i] << " ";
      }
      os << ")";
    }

    os << "\n";
  }

  os << "  Entries: " << std::dec << capacity << "\n";
  os << "================================================\n";
}

}; // namespace sycon