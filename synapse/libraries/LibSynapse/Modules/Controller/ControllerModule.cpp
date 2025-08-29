#include <LibSynapse/Modules/Controller/ControllerModule.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

const Tofino::TofinoContext *ControllerModuleFactory::get_tofino_ctx(const EP *ep) const {
  const Context &ctx                      = ep->get_ctx();
  const Tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<Tofino::TofinoContext>(TargetType(TargetArchitecture::Tofino, target.instance_id));
  return tofino_ctx;
}

Tofino::TofinoContext *ControllerModuleFactory::get_mutable_tofino_ctx(EP *ep) const {
  Context &ctx                      = ep->get_mutable_ctx();
  Tofino::TofinoContext *tofino_ctx = ctx.get_mutable_target_ctx<Tofino::TofinoContext>(TargetType(TargetArchitecture::Tofino, target.instance_id));
  return tofino_ctx;
}

Tofino::TNA &ControllerModuleFactory::get_mutable_tna(EP *ep) const {
  Tofino::TofinoContext *ctx = get_mutable_tofino_ctx(ep);
  return ctx->get_mutable_tna();
}

const Tofino::TNA &ControllerModuleFactory::get_tna(const EP *ep) const {
  const Tofino::TofinoContext *ctx = get_tofino_ctx(ep);
  return ctx->get_tna();
}

} // namespace Controller
} // namespace LibSynapse