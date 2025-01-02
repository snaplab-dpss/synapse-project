#pragma once

#include "../../execution_plan/context.h"

namespace synapse {
namespace tofino_cpu {

class TofinoCPUContext : public TargetContext {
public:
  TofinoCPUContext() {}

  virtual TargetContext *clone() const override { return new TofinoCPUContext(*this); }
};

} // namespace tofino_cpu

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(tofino_cpu, TofinoCPUContext)
} // namespace synapse