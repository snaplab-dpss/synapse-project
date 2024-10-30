#pragma once

#include "data_structures/data_structures.h"
#include "../context.h"

namespace x86 {

class x86Context : public TargetContext {
public:
  x86Context() {}

  virtual TargetContext *clone() const override {
    return new x86Context(*this);
  }
};

} // namespace x86