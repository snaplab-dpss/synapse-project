#include "../../include/sycon/primitives/register.h"

#include "../../include/sycon/log.h"

namespace sycon {

Register::Register(const std::string &control_name,
                   const std::string &register_name)
    : Table(control_name, register_name) {
  init_key({{"$REGISTER_INDEX", &index}});
  init_data({{control_name + ".f1", &content}});
}

void Register::set(u32 i, u32 value) {
  session->beginBatch();

  key_setup(i);
  data_setup(value);

  auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status)

  auto block = true;
  session->endBatch(block);
}

void Register::overwrite_all_entries(u32 value) {
  auto size = get_size();
  session->beginBatch();

  data_setup(value);

  for (size_t i = 0; i < size; i++) {
    key_setup(i);

    auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status)
  }

  auto block = true;
  session->endBatch(block);
}

void Register::key_setup(u32 i) {
  table->keyReset(key.get());

  auto bf_status = key->setValue(index, static_cast<u64>(i));
  ASSERT_BF_STATUS(bf_status)
}

void Register::data_setup(u32 value) {
  table->dataReset(data.get());

  auto bf_status = data->setValue(content, static_cast<u64>(value));
  ASSERT_BF_STATUS(bf_status)
}

};  // namespace sycon