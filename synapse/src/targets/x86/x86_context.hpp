#pragma once

#include "data_structures/data_structures.hpp"
#include "../../execution_plan/context.hpp"

namespace synapse {
namespace x86 {

class x86Context : public TargetContext {
public:
  x86Context() {}

  virtual TargetContext *clone() const override { return new x86Context(*this); }
};

} // namespace x86

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(x86, x86Context)
} // namespace synapse