#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

LPM *TofinoModuleFactory::build_lpm(const EP *ep, const BDDNode *node, addr_t obj) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const DS_ID id                     = "lpm_" + std::to_string(obj);
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  LPM *lpm = new LPM(id, properties.total_ports);

  if (!tofino_ctx->can_place(ep, node, lpm)) {
    delete lpm;
    lpm = nullptr;
  }

  return lpm;
}

bool TofinoModuleFactory::can_build_lpm(const EP *ep, const BDDNode *node, addr_t obj) {
  LPM *lpm = build_lpm(ep, node, obj);

  if (!lpm) {
    return false;
  }

  delete lpm;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse