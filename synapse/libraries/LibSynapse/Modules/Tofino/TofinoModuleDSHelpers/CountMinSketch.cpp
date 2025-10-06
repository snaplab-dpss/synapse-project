#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

CountMinSketch *build_cms(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                          u32 width, u32 height) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  const DS_ID id                     = TofinoModuleFactory::build_cms_id(obj);
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  CountMinSketch *cms = new CountMinSketch(properties, id, keys_sizes, width, height);

  if (!tofino_ctx->can_place(ep, node, cms)) {
    delete cms;
    cms = nullptr;
  }

  return cms;
}

CountMinSketch *reuse_cms(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::CountMinSketch && "Invalid DS type");

  CountMinSketch *cms = dynamic_cast<CountMinSketch *>(*ds.begin());

  if (!tofino_ctx->can_place(ep, node, cms)) {
    return nullptr;
  }

  return cms;
}

} // namespace

std::string TofinoModuleFactory::build_cms_id(addr_t obj) { return "cms_" + std::to_string(obj); }

bool TofinoModuleFactory::can_build_or_reuse_cms(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                 const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height) {
  CountMinSketch *cms = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_CountMinSketch);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>(type);
    const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::CountMinSketch && "Invalid DS type");

    cms = dynamic_cast<CountMinSketch *>(*ds.begin());

    if (!tofino_ctx->can_place(ep, node, cms)) {
      cms = nullptr;
      return false;
    }

    if (cms->width != width || cms->height != height) {
      return false;
    }

    return true;
  }

  cms = build_cms(ep, node, type, obj, keys, width, height);

  if (!cms) {
    return false;
  }

  delete cms;
  return true;
}

CountMinSketch *TofinoModuleFactory::build_or_reuse_cms(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                        const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height) {
  CountMinSketch *cms = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_CountMinSketch)) {
    cms = reuse_cms(ep, node, type, obj);
  } else {
    cms = build_cms(ep, node, type, obj, keys, width, height);
  }

  return cms;
}

} // namespace Tofino
} // namespace LibSynapse
