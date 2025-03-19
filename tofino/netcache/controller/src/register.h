#pragma once

#include "table.h"

namespace netcache {

class Register : public Table {
private:
  bf_rt_id_t index;
  bf_rt_id_t content;

protected:
  Register(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, const bf_rt_target_t &dev_tgt,
           const std::string &register_name)
      : Table(info, session, dev_tgt, register_name) {
    init_key({{"$REGISTER_INDEX", &index}});
    init_data({{register_name + ".f1", &content}});
  }

public:
  void set(uint16_t i, uint32_t value) {
    key_setup(i);
    data_setup(value);

    auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  std::vector<uint64_t> get(uint16_t i, bool from_hw = false) {
    auto hw_flag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(i);
    table->dataReset(data.get());

    auto bf_status = table->tableEntryGet(*session, dev_tgt, *key, hw_flag, data.get());
    ASSERT_BF_STATUS(bf_status);

    // The output is a vector, one value per pipeline
    std::vector<uint64_t> values;
    bf_status = data->getValue(content, &values);
    ASSERT_BF_STATUS(bf_status);

    return values;
  }

  uint64_t get_min(uint16_t i, bool from_hw = false) {
    auto values = get(i, from_hw);
    return *std::min_element(values.begin(), values.end());
  }

  uint64_t get_max(uint16_t i, bool from_hw = false) {
    auto values = get(i, from_hw);
    return *std::max_element(values.begin(), values.end());
  }

  void overwrite_all_entries(uint32_t value) {
    auto size = get_size();

    data_setup(value);

    for (size_t i = 0; i < size; i++) {
      key_setup(i);

      auto bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
      ASSERT_BF_STATUS(bf_status);
    }
  }

private:
  void key_setup(uint32_t i) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(index, static_cast<uint64_t>(i));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(uint32_t value) {
    table->dataReset(data.get());

    auto bf_status = data->setValue(content, static_cast<uint64_t>(value));
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace netcache
