#include "LibSynapse/Target.h"
#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

DchainTable *build_dchain_table(const EP *ep, const BDDNode *node, const TargetType type, const dchain_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  assert(data.key->getWidth() == klee::Expr::Int32);

  DS_ID id = "dchain_table_" + std::to_string(data.obj);

  DchainTable *dchain_table = new DchainTable(id, data.capacity);
  dchain_table->add_table(node->get_id());

  if (!tofino_ctx->can_place(ep, node, dchain_table)) {
    delete dchain_table;
    dchain_table = nullptr;
  }

  return dchain_table;
}

DchainTable *get_dchain_table(const EP *ep, const BDDNode *node, const TargetType type, const dchain_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No dchain table found");
  assert(ds.size() == 1);
  DS *vt = *ds.begin();

  assert(vt->type == DSType::DchainTable && "Unexpected type");
  return dynamic_cast<DchainTable *>(vt);
}

bool can_reuse_dchain_table(const EP *ep, const BDDNode *node, const TargetType type, const dchain_table_data_t &data) {
  DchainTable *dchain_table = get_dchain_table(ep, node, type, data);
  assert(dchain_table && "Dchain table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);
  assert(!dchain_table->has_table(node->get_id()));

  DchainTable *clone = dynamic_cast<DchainTable *>(dchain_table->clone());

  clone->add_table(node->get_id());
  dchain_table = clone;

  bool can_place = tofino_ctx->can_place(ep, node, dchain_table);
  delete dchain_table;

  return can_place;
}

DchainTable *reuse_dchain_table(const EP *ep, const BDDNode *node, const TargetType type, const dchain_table_data_t &data) {
  DchainTable *dchain_table = get_dchain_table(ep, node, type, data);
  assert(dchain_table && "Dchain table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);
  assert(!dchain_table->has_table(node->get_id()));

  dchain_table->add_table(node->get_id());

  if (!tofino_ctx->can_place(ep, node, dchain_table)) {
    return nullptr;
  }

  return dchain_table;
}

} // namespace

DchainTable *TofinoModuleFactory::build_or_reuse_dchain_table(const EP *ep, const BDDNode *node, const TargetType type,
                                                              const dchain_table_data_t &data) {
  DchainTable *dchain_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  if (already_placed) {
    dchain_table = reuse_dchain_table(ep, node, type, data);
  } else {
    dchain_table = build_dchain_table(ep, node, type, data);
  }

  return dchain_table;
}

bool TofinoModuleFactory::can_build_or_reuse_dchain_table(const EP *ep, const BDDNode *node, const TargetType type, const dchain_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  if (already_placed) {
    return can_reuse_dchain_table(ep, node, type, data);
  }

  DchainTable *dchain_table = build_dchain_table(ep, node, type, data);

  if (!dchain_table) {
    return false;
  }

  delete dchain_table;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse
