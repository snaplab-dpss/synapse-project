#include "../../include/sycon/primitives/register.h"

#include "../../include/sycon/log.h"

namespace sycon {

PrimitiveRegister::PrimitiveRegister(const std::string &control_name, const std::string &register_name)
    : PrimitiveTable(control_name, register_name) {
  init_key({{"$REGISTER_INDEX", &index_id}});
  init_data({{append_control(control_name, register_name) + ".f1", &value_id}});

  bf_status_t bf_status = table->dataFieldSizeGet(value_id, &value_size);
  ASSERT_BF_STATUS(bf_status);
}

PrimitiveRegister::PrimitiveRegister(const PrimitiveRegister &other)
    : PrimitiveTable(other), index_id(other.index_id), value_id(other.value_id), value_size(other.value_size), pipes(other.pipes) {
  init_key({{"$REGISTER_INDEX", &index_id}});
  init_data({{append_control(control, name) + ".f1", &value_id}});
}

u32 PrimitiveRegister::get(u32 i) {
  key_setup(i);
  data_reset();

  bfrt::BfRtTable::BfRtTableGetFlag flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  bf_status_t bf_status                  = table->tableEntryGet(*session, dev_tgt, *key, flag, data.get());
  ASSERT_BF_STATUS(bf_status);

  u64 value;
  bf_status = data->getValue(value_id, &value);
  ASSERT_BF_STATUS(bf_status);

  return value;
}

void PrimitiveRegister::set(u32 i, u32 value) {
  key_setup(i);
  data_setup(value);

  bf_status_t bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status);
}

void PrimitiveRegister::overwrite_all_entries(u32 value) {
  data_setup(value);

  for (size_t i = 0; i < capacity; i++) {
    key_setup(i);

    bf_status_t bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }
}

void PrimitiveRegister::key_setup(u32 i) {
  table->keyReset(key.get());

  bf_status_t bf_status = key->setValue(index_id, static_cast<u64>(i));
  ASSERT_BF_STATUS(bf_status);
}

void PrimitiveRegister::data_setup(u32 value) {
  data_reset();

  bf_status_t bf_status = data->setValue(value_id, static_cast<uint64_t>(value));
  ASSERT_BF_STATUS(bf_status);
}

void PrimitiveRegister::data_reset() { table->dataReset(data.get()); }

bits_t PrimitiveRegister::get_value_size() const { return value_size; }

void PrimitiveRegister::dump(std::ostream &os) const {
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

    os << "\n";
  }

  os << "  Entries: " << std::dec << capacity << "\n";
  os << "================================================\n";
}

}; // namespace sycon