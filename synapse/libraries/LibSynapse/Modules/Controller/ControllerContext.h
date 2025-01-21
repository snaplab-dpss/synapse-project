#pragma once

#include <LibSynapse/Context.h>

namespace LibSynapse {
namespace Controller {

class ControllerContext : public TargetContext {
public:
  ControllerContext() {}

  virtual TargetContext *clone() const override { return new ControllerContext(*this); }
};

} // namespace Controller

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(Controller, ControllerContext)

} // namespace LibSynapse