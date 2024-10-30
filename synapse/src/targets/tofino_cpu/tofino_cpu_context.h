#pragma once

#include "../context.h"

namespace tofino_cpu {

class TofinoCPUContext : public TargetContext {
public:
  TofinoCPUContext() {}

  virtual TargetContext *clone() const override {
    return new TofinoCPUContext(*this);
  }
};

} // namespace tofino_cpu
