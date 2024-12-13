#include "tofino_cpu_context.h"

template <>
const tofino_cpu::TofinoCPUContext *
Context::get_target_ctx<tofino_cpu::TofinoCPUContext>() const {
  TargetType type = TargetType::TofinoCPU;
  ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<const tofino_cpu::TofinoCPUContext *>(
      target_ctxs.at(type));
}

template <>
tofino_cpu::TofinoCPUContext *
Context::get_mutable_target_ctx<tofino_cpu::TofinoCPUContext>() {
  TargetType type = TargetType::TofinoCPU;
  ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<tofino_cpu::TofinoCPUContext *>(target_ctxs.at(type));
}