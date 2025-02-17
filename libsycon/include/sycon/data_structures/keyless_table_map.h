#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"

namespace sycon {

template <size_t V> class KeylessTableMap : public PrimitiveTable {
  static_assert(V > 0);

private:
  table_value_t<V> entry;
  bf_rt_id_t action_id;
  std::optional<time_ms_t> timeout;
  std::optional<bfrt::BfRtIdleTmoExpiryCb> callback;

public:
  KeylessTableMap(const std::string &_control_name, const std::string &_table_name, const table_value_t<V> &_entry)
      : PrimitiveTable(_control_name, _table_name) {
    init_key();
    init_action(&action_id);
    init_data_with_action(action_id);

    entry = _entry;
    driver_set();
  }

private:
  void data_setup(const table_value_t<V> &v) {
    bf_status_t bf_status = table->dataReset(action_id, data.get());
    ASSERT_BF_STATUS(bf_status)

    assert(V == data_fields.size());

    for (size_t i = 0; i < V; i++) {
      auto param_field_value = v.values[i];
      auto param_field       = data_fields[i];
      bf_status              = data->setValue(param_field.id, param_field_value);
      ASSERT_BF_STATUS(bf_status)
    }

    if (time_aware) {
      bf_status = data->setValue(entry_ttl_data_id, static_cast<u64>(*timeout));
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void driver_set() {
    data_setup(entry);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    bf_status_t bf_status = table->tableDefaultEntrySet(*session, dev_tgt, *data);
    ASSERT_BF_STATUS(bf_status)
  }
};

} // namespace sycon