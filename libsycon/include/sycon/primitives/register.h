#pragma once

#include "meta_table.h"

namespace sycon {

// A P4 Register<T, _>. T is either a plain bit<N> (bfrt data field "<name>.f1") or a struct of
// two fields (bfrt data fields "<name>.lo" and "<name>.hi", e.g. the pair used by
// read-conditional-write-return-other actions). For a pair, get/set operate on `lo` (the value
// half); get_pair_max/set_pair handle both halves.
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
  u32 get(u32 i, u16 pipe);
  u32 get_max(u32 i);
  u32 get_min(u32 i);

  void set(u32 i, u32 value);
  void set(u32 i, u32 value, u16 pipe_id);

  bool is_paired() const { return paired; }
  // Both halves of a pair cell, each the maximum over the pipes (see VectorRegister::get).
  std::pair<u32, u32> get_pair_max(u32 i);
  // Both halves of a pair cell as one pipe holds them.
  std::pair<u32, u32> get_pair(u32 i, u16 pipe);
  // The pipe a port belongs to: 128 ports per pipe.
  static u16 pipe_of(u16 dev_port) { return dev_port >> 7; }
  void set_pair(u32 i, u32 lo, u32 hi);
  // Resets every entry to the register's P4 initial value (0 for every register synapse emits).
  void reset_all_entries();

  bits_t get_value_size() const;
  virtual void dump(std::ostream &) const override;

private:
  void init_fields();
  void key_setup(u32 i);
  void data_setup(u32 value);
  void data_reset();
};

}; // namespace sycon