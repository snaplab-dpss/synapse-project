#pragma once

#include "table.h"

namespace sycon {

class Register : public Table {
 private:
  bf_rt_id_t index;
  bf_rt_id_t content;

 protected:
  Register(const std::string &control_name, const std::string &register_name);

  void set(uint32_t i, uint32_t value);
  void overwrite_all_entries(uint32_t value);

 private:
  void key_setup(uint32_t i);
  void data_setup(uint32_t value);
};

};  // namespace sycon