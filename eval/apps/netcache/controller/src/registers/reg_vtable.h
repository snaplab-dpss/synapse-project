#pragma once

#include "../register.h"

namespace netcache {

class RegVTable : public Register {
 public:
  RegVTable(const bfrt::BfRtInfo *info,
             std::shared_ptr<bfrt::BfRtSession> session,
             const bf_rt_target_t &dev_tgt)
      : Register(info, session, dev_tgt, "Ingress.reg_vtable") {}

  void set_all_true() { overwrite_all_entries(1); }
  void set_all_false() { overwrite_all_entries(0); }

  void allocate(uint16_t index, uint32_t val) { set(index, val); }
  void retrieve(uint16_t index, bool from_hw = false) { get(index, from_hw); }
};

};  // namespace netcache
