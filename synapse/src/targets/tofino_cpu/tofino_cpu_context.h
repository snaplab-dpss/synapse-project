#pragma once

#include "../context.h"

namespace tofino_cpu {

class TofinoCPUContext : public TargetContext {
private:
  u64 capacity_pps;

public:
  TofinoCPUContext() : capacity_pps(100'000) {}

  virtual TargetContext *clone() const override {
    return new TofinoCPUContext(*this);
  }

  virtual u64 estimate_throughput_pps() const override {
    return capacity_pps;
  }
};

} // namespace tofino_cpu
