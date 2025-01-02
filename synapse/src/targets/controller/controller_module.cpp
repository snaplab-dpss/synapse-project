#include "controller_module.h"

#include "../../execution_plan/execution_plan.h"

namespace synapse {
namespace controller {

const tofino::TofinoContext *ControllerModuleFactory::get_tofino_ctx(const EP *ep) const {
  const Context &ctx = ep->get_ctx();
  const tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<tofino::TofinoContext>();
  return tofino_ctx;
}

tofino::TofinoContext *ControllerModuleFactory::get_mutable_tofino_ctx(EP *ep) const {
  Context &ctx = ep->get_mutable_ctx();
  tofino::TofinoContext *tofino_ctx = ctx.get_mutable_target_ctx<tofino::TofinoContext>();
  return tofino_ctx;
}

tofino::TNA &ControllerModuleFactory::get_mutable_tna(EP *ep) const {
  tofino::TofinoContext *ctx = get_mutable_tofino_ctx(ep);
  return ctx->get_mutable_tna();
}

const tofino::TNA &ControllerModuleFactory::get_tna(const EP *ep) const {
  const tofino::TofinoContext *ctx = get_tofino_ctx(ep);
  return ctx->get_tna();
}

} // namespace controller
} // namespace synapse