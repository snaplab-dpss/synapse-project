#pragma once

#include "table.h"

namespace sycon {

class Register : public Table {
 private:
  bf_rt_id_t index;
  bf_rt_id_t content;

 public:
  Register(const std::string &control_name, const std::string &register_name);

  u32 get(u32 i);
  void set(u32 i, u32 value);
  void overwrite_all_entries(u32 value);

 private:
  void key_setup(u32 i);
  void data_setup(u32 value);
  void data_reset();
};

};  // namespace sycon