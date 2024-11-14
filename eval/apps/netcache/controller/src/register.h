#pragma once

#include "table.h"

namespace peregrine {

class Register : public Table {
 private:
  bf_rt_id_t index;
  bf_rt_id_t content;

 protected:
  Register(const bfrt::BfRtInfo *info,
           std::shared_ptr<bfrt::BfRtSession> session,
           const bf_rt_target_t &dev_tgt, const std::string &register_name)
      : Table(info, session, dev_tgt, register_name) {
    init_key({{"$REGISTER_INDEX", &index}});
    init_data({{register_name + ".f1", &content}});
  }

  void set(uint32_t i, uint32_t value) {
    session->beginBatch();

    key_setup(i);
    data_setup(value);

    auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    assert(bf_status == BF_SUCCESS);

    auto block = true;
    session->endBatch(block);
  }

  void overwrite_all_entries(uint32_t value) {
    auto size = get_size();
    session->beginBatch();

    data_setup(value);

    for (size_t i = 0; i < size; i++) {
      key_setup(i);

      auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
      assert(bf_status == BF_SUCCESS);
    }

    auto block = true;
    session->endBatch(block);
  }

 private:
  void key_setup(uint32_t i) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(index, static_cast<uint64_t>(i));
    assert(bf_status == BF_SUCCESS);
  }

  void data_setup(uint32_t value) {
    table->dataReset(data.get());

    auto bf_status = data->setValue(content, static_cast<uint64_t>(value));
    assert(bf_status == BF_SUCCESS);
  }
};

};  // namespace nat