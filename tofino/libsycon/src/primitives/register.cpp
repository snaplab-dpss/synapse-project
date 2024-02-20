#include "../../include/sycon/primitives/register.h"

#include "../../include/sycon/log.h"

namespace sycon {

Register::Register(const std::string &control_name,
                   const std::string &register_name)
    : Table(control_name, register_name) {
  init_key({{"$REGISTER_INDEX", &index}});
  init_data({{control_name + ".f1", &content}});
}

void Register::set(uint32_t i, uint32_t value) {
  session->beginBatch();

  key_setup(i);
  data_setup(value);

  auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status)

  auto block = true;
  session->endBatch(block);
}

void Register::overwrite_all_entries(uint32_t value) {
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

void Register::key_setup(uint32_t i) {
  table->keyReset(key.get());

  auto bf_status = key->setValue(index, static_cast<uint64_t>(i));
  ASSERT_BF_STATUS(bf_status)
}

void Register::data_setup(uint32_t value) {
  table->dataReset(data.get());

  auto bf_status = data->setValue(content, static_cast<uint64_t>(value));
  ASSERT_BF_STATUS(bf_status)
}

};  // namespace sycon