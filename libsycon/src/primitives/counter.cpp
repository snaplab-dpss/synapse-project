#include "../../include/sycon/primitives/counter.h"

#include "../../include/sycon/log.h"

namespace sycon {

Counter::Counter(const std::string &control_name, const std::string &register_name, bool count_bytes, bool count_packets)
    : Table(control_name, register_name) {
  assert(count_bytes || count_packets);

  init_key({{"$COUNTER_INDEX", &index}});

  std::unordered_map<std::string, bf_rt_id_t *> fields;

  if (count_bytes) {
    bytes                         = 0;
    fields["$COUNTER_SPEC_BYTES"] = &(*bytes);
  }

  if (count_packets) {
    packets                      = 0;
    fields["$COUNTER_SPEC_PKTS"] = &(*packets);
  }

  init_data(fields);
  assert(bytes || packets);
}

counter_data_t Counter::get(u32 i) {
  key_setup(i);
  data_reset();

  auto flag             = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, flag, data.get());
  ASSERT_BF_STATUS(bf_status)

  counter_data_t values;

  if (bytes) {
    bf_status = data->getValue(*bytes, &values.bytes);
    ASSERT_BF_STATUS(bf_status)
  }

  if (packets) {
    bf_status = data->getValue(*packets, &values.packets);
    ASSERT_BF_STATUS(bf_status)
  }

  return values;
}

void Counter::reset(u32 i) {
  key_setup(i);
  data_setup(0, 0);

  bf_status_t bf_status = table->tableEntryMod(*session, dev_tgt, *key, *data);
  ASSERT_BF_STATUS(bf_status)
}

void Counter::reset() {
  auto size = get_size();
  for (size_t i = 0; i < size; i++) {
    reset(i);
  }
}

void Counter::key_setup(u32 i) {
  table->keyReset(key.get());

  bf_status_t bf_status = key->setValue(index, static_cast<u64>(i));
  ASSERT_BF_STATUS(bf_status)
}

void Counter::data_setup(u64 value_bytes, u64 value_packets) {
  data_reset();

  if (bytes) {
    bf_status_t bf_status = data->setValue(*bytes, static_cast<u64>(value_bytes));
    ASSERT_BF_STATUS(bf_status)
  }

  if (packets) {
    bf_status_t bf_status = data->setValue(*packets, static_cast<u64>(value_packets));
    ASSERT_BF_STATUS(bf_status)
  }
}

void Counter::data_reset() { table->dataReset(data.get()); }

}; // namespace sycon