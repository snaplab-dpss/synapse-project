#include <LibTessera/Modules/Tofino/TofinoModule.h>
#include <LibTessera/Modules/Tofino/TofinoContext.h>
#include <LibTessera/ExecutionPlan.h>

#include <unordered_set>

namespace LibTessera {
namespace Tofino {

namespace {

BloomFilter *build_bf(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  const DS_ID id                     = TofinoModuleFactory::build_bf_id(obj);
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  BloomFilter *bf = new BloomFilter(properties, id, keys_sizes, width, height);

  if (!tofino_ctx->can_place(ep, node, bf)) {
    delete bf;
    bf = nullptr;
  }

  return bf;
}

BloomFilter *reuse_bf(const EP *ep, const BDDNode *node, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::BloomFilter && "Invalid DS type");

  BloomFilter *bf = dynamic_cast<BloomFilter *>(*ds.begin());

  if (!tofino_ctx->can_place(ep, node, bf)) {
    return nullptr;
  }

  return bf;
}

} // namespace

DS_ID TofinoModuleFactory::build_bf_id(addr_t obj) { return "bf_" + std::to_string(obj); }

bool TofinoModuleFactory::can_build_or_reuse_bf(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                                u32 width, u32 height) {
  BloomFilter *bf = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_BloomFilter);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::BloomFilter && "Invalid DS type");

    bf = dynamic_cast<BloomFilter *>(*ds.begin());

    if (!tofino_ctx->can_place(ep, node, bf)) {
      bf = nullptr;
      return false;
    }

    if (bf->width != width || bf->height != height) {
      return false;
    }

    return true;
  }

  bf = build_bf(ep, node, obj, keys, width, height);

  if (!bf) {
    return false;
  }

  delete bf;
  return true;
}

BloomFilter *TofinoModuleFactory::build_or_reuse_bf(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                                    u32 width, u32 height) {
  BloomFilter *bf = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_BloomFilter)) {
    bf = reuse_bf(ep, node, obj);
  } else {
    bf = build_bf(ep, node, obj, keys, width, height);
  }

  return bf;
}

} // namespace Tofino
} // namespace LibTessera