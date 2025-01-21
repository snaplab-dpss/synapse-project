#pragma once

#include <LibSynapse/Modules/x86/DataStructures/DataStructures.h>
#include <LibSynapse/Context.h>

namespace LibSynapse {
namespace x86 {

class x86Context : public TargetContext {
public:
  x86Context() {}

  virtual TargetContext *clone() const override { return new x86Context(*this); }
};

} // namespace x86

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(x86, x86Context)

} // namespace LibSynapse