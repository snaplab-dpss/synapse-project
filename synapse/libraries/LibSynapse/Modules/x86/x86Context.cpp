#include <LibSynapse/Modules/x86/x86Context.h>

namespace LibSynapse {

template <> const x86::x86Context *Context::get_target_ctx<x86::x86Context>() const {
  TargetType type = TargetType::x86;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<const x86::x86Context *>(target_ctxs.at(type));
}

template <> x86::x86Context *Context::get_mutable_target_ctx<x86::x86Context>() {
  TargetType type = TargetType::x86;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<x86::x86Context *>(target_ctxs.at(type));
}

} // namespace LibSynapse