#include "../../include/sycon/primitives/register.hpp"

#include "../../include/sycon/log.hpp"

namespace sycon {

Register::Register(const std::string &control_name, const std::string &register_name)
    : Table(control_name, register_name) {
  init_key({{"$REGISTER_INDEX", &index}});
  init_data({{append_control(control_name, register_name) + ".f1", &content}});
}

u32 Register::get(u32 i) {
  key_setup(i);
  data_reset();

  auto flag             = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, flag, data.get());
  ASSERT_BF_STATUS(bf_status)

  u64 value;
  bf_status = data->getValue(content, &value);
  ASSERT_BF_STATUS(bf_status)

  return value;
}

void Register::set(u32 i, u32 value) {
  key_setup(i);
  data_setup(value);

  auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status)
}

void Register::overwrite_all_entries(u32 value) {
  auto size = get_size();

  data_setup(value);

  for (size_t i = 0; i < size; i++) {
    key_setup(i);

    auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status)
  }
}

void Register::key_setup(u32 i) {
  table->keyReset(key.get());

  auto bf_status = key->setValue(index, static_cast<u64>(i));
  ASSERT_BF_STATUS(bf_status)
}

void Register::data_setup(u32 value) {
  data_reset();

  auto bf_status = data->setValue(content, static_cast<u64>(value));
  ASSERT_BF_STATUS(bf_status)
}

void Register::data_reset() { table->dataReset(data.get()); }

}; // namespace sycon