#pragma once

#include <optional>

#include "table.h"

namespace sycon {

struct counter_data_t {
  u64 bytes;
  u64 packets;
};

class PrimitiveCounter : public PrimitiveTable {
private:
  bf_rt_id_t index;

  std::optional<bf_rt_id_t> bytes;
  std::optional<bf_rt_id_t> packets;

public:
  PrimitiveCounter(const std::string &control_name, const std::string &counter_name, bool count_bytes, bool count_packets);
  PrimitiveCounter(const PrimitiveCounter &other) = default;
  PrimitiveCounter(PrimitiveCounter &&other)      = default;

  counter_data_t get(u32 i);

  void reset(u32 i);
  void reset();

private:
  void key_setup(u32 i);
  void data_setup(u64 value_bytes, u64 value_packets);
  void data_reset();
};

}; // namespace sycon