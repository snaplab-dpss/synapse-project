#pragma once

#include "table.h"

namespace sycon {

class PrimitiveRegister : public PrimitiveTable {
private:
  bf_rt_id_t index_id;
  bf_rt_id_t value_id;
  bits_t value_size;
  size_t pipes;

public:
  PrimitiveRegister(const std::string &control_name, const std::string &register_name);
  PrimitiveRegister(const PrimitiveRegister &other);
  PrimitiveRegister(PrimitiveRegister &&other) = delete;

  u32 get(u32 i);
  void set(u32 i, u32 value);
  void overwrite_all_entries(u32 value);

  bits_t get_value_size() const;
  virtual void dump(std::ostream &) const override;

private:
  void key_setup(u32 i);
  void data_setup(u32 value);
  void data_reset();
};

}; // namespace sycon