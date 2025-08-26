#include <LibSynapse/Modules/Controller/ControllerContext.h>

namespace LibSynapse {

template <> const Controller::ControllerContext *Context::get_target_ctx<Controller::ControllerContext>() const {
  TargetArchitecture type = TargetArchitecture::Controller;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<const Controller::ControllerContext *>(target_ctxs.at(type));
}

template <> Controller::ControllerContext *Context::get_mutable_target_ctx<Controller::ControllerContext>() {
  TargetArchitecture type = TargetArchitecture::Controller;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<Controller::ControllerContext *>(target_ctxs.at(type));
}

} // namespace LibSynapse