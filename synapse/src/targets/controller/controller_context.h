#pragma once

#include "../../execution_plan/context.h"

namespace synapse {
namespace controller {

class ControllerContext : public TargetContext {
public:
  ControllerContext() {}

  virtual TargetContext *clone() const override { return new ControllerContext(*this); }
};

} // namespace controller

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(controller, ControllerContext)
} // namespace synapse