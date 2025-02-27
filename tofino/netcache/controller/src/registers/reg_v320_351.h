#pragma once

#include "../register.h"

namespace netcache {

class RegV320_351 : public Register {
 public:
  RegV320_351(const bfrt::BfRtInfo *info,
             std::shared_ptr<bfrt::BfRtSession> session,
             const bf_rt_target_t &dev_tgt)
      : Register(info, session, dev_tgt, "SwitchIngress.reg_v320_351") {}

  void set_all_true() { overwrite_all_entries(1); }
  void set_all_false() { overwrite_all_entries(0); }

  void allocate(uint16_t index, uint32_t val) { set(index, val); }
  uint32_t retrieve(uint16_t index, bool from_hw = false) { return get(index, from_hw); }
};

};  // namespace netcache
