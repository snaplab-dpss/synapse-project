#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

LPM *TofinoModuleFactory::build_lpm(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> original_key,
                                    const std::vector<klee::ref<klee::Expr>> &keys) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const DS_ID id                  = "lpm_" + std::to_string(obj);
  const lpm_config_t &cfg         = ep->get_ctx().get_lpm_config(obj);

  std::vector<bits_t> keys_size;
  bits_t key_size = 0;
  for (klee::ref<klee::Expr> key : keys) {
    keys_size.push_back(key->getWidth());
    key_size += key->getWidth();
  }
  assert_or_panic(key_size == cfg.key_size, "LPM %lu: the lookup key is %u bits, the allocation says %u", obj, key_size, cfg.key_size);

  LPM *lpm = new LPM(id, cfg.capacity, keys_size, ep->get_ctx().is_dns_name(original_key));

  if (!tofino_ctx->can_place(ep, node, lpm)) {
    delete lpm;
    lpm = nullptr;
  }

  return lpm;
}

bool TofinoModuleFactory::can_build_lpm(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> original_key,
                                        const std::vector<klee::ref<klee::Expr>> &keys) {
  LPM *lpm = build_lpm(ep, node, obj, original_key, keys);

  if (!lpm) {
    return false;
  }

  delete lpm;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse