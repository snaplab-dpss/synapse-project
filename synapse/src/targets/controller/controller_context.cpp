#include "controller_context.h"

namespace synapse {
template <>
const ctrl::ControllerContext *Context::get_target_ctx<ctrl::ControllerContext>() const {
  TargetType type = TargetType::Controller;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<const ctrl::ControllerContext *>(target_ctxs.at(type));
}

template <> ctrl::ControllerContext *Context::get_mutable_target_ctx<ctrl::ControllerContext>() {
  TargetType type = TargetType::Controller;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<ctrl::ControllerContext *>(target_ctxs.at(type));
}
} // namespace synapse