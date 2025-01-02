#include "controller_context.h"

namespace synapse {
template <>
const controller::ControllerContext *
Context::get_target_ctx<controller::ControllerContext>() const {
  TargetType type = TargetType::Controller;
  SYNAPSE_ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<const controller::ControllerContext *>(target_ctxs.at(type));
}

template <>
controller::ControllerContext *
Context::get_mutable_target_ctx<controller::ControllerContext>() {
  TargetType type = TargetType::Controller;
  SYNAPSE_ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<controller::ControllerContext *>(target_ctxs.at(type));
}
} // namespace synapse