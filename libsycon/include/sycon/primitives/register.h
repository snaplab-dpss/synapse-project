#pragma once

#include "meta_table.h"

namespace sycon {

class Register : public MetaTable {
private:
  bf_rt_id_t index_id;
  bf_rt_id_t value_id;
  bits_t value_size;
  size_t pipes;

public:
  Register(const std::string &name);
  Register(const Register &other);
  Register(Register &&other) = delete;

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