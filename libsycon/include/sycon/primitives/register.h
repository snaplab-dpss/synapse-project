#pragma once

#include "meta_table.h"

namespace sycon {

// A P4 Register<T, _>. T is either a plain bit<N> (bfrt data field "<name>.f1") or a struct of
// two fields (bfrt data fields "<name>.lo" and "<name>.hi", e.g. the pair used by
// read-conditional-write-return-other actions). For a pair, get/set operate on `lo` (the value
// half) and `hi` is only reported by dump().
class Register : public MetaTable {
private:
  bf_rt_id_t index_id;
  bf_rt_id_t value_id;
  bf_rt_id_t hi_id;
  bool paired;
  bits_t value_size;
  size_t pipes;

public:
  Register(const std::string &name);
  Register(const Register &other);
  Register(Register &&other) = delete;

  std::vector<u32> get_per_pipe(u32 i);
  u32 get_max(u32 i);
  u32 get_min(u32 i);

  void set(u32 i, u32 value);
  void set(u32 i, u32 value, u16 pipe_id);
  void overwrite_all_entries(u32 value);

  bits_t get_value_size() const;
  virtual void dump(std::ostream &) const override;

private:
  void init_fields();
  void key_setup(u32 i);
  void data_setup(u32 value);
  void data_reset();
};

}; // namespace sycon