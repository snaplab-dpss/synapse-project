#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

GuardedMapTable *build_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>(target);
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  bits_t key_size = 0;
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    key_size += key->getWidth();
    keys_size.push_back(key->getWidth());
  }

  assert(data.value->getWidth() == klee::Expr::Int32);

  const DS_ID id = "guarded_map_table_" + std::to_string(data.obj);

  GuardedMapTable *guarded_map_table = new GuardedMapTable(properties, id, data.capacity, key_size);
  guarded_map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->can_place(ep, node, guarded_map_table)) {
    delete guarded_map_table;
    guarded_map_table = nullptr;
  }

  return guarded_map_table;
}

GuardedMapTable *get_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(target);

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No guarded map table found");
  assert(ds.size() == 1);
  DS *mt = *ds.begin();

  assert(mt->type == DSType::GuardedMapTable && "Unexpected type");
  return dynamic_cast<GuardedMapTable *>(mt);
}

bool can_reuse_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target, const map_table_data_t &data) {
  GuardedMapTable *guarded_map_table = get_guarded_map_table(ep, node, target, data);
  assert(guarded_map_table && "Guarded map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(target);
  assert(!guarded_map_table->has_table(node->get_id()));

  GuardedMapTable *clone = dynamic_cast<GuardedMapTable *>(guarded_map_table->clone());

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  clone->add_table(node->get_id(), keys_size, data.time_aware);
  guarded_map_table = clone;

  const bool can_place = tofino_ctx->can_place(ep, node, guarded_map_table);
  delete guarded_map_table;

  return can_place;
}

GuardedMapTable *reuse_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target, const map_table_data_t &data) {
  GuardedMapTable *guarded_map_table = get_guarded_map_table(ep, node, target, data);
  assert(guarded_map_table && "Guarded map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(target);
  assert(!guarded_map_table->has_table(node->get_id()));

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  guarded_map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->can_place(ep, node, guarded_map_table)) {
    return nullptr;
  }

  return guarded_map_table;
}

} // namespace

GuardedMapTable *TofinoModuleFactory::build_or_reuse_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target,
                                                                       const map_table_data_t &data) {
  GuardedMapTable *guarded_map_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable);

  if (already_placed) {
    guarded_map_table = reuse_guarded_map_table(ep, node, target, data);
  } else {
    guarded_map_table = build_guarded_map_table(ep, node, target, data);
  }

  return guarded_map_table;
}

bool TofinoModuleFactory::can_build_or_reuse_guarded_map_table(const EP *ep, const BDDNode *node, const TargetType target,
                                                               const map_table_data_t &data) {
  const Context &ctx        = ep->get_ctx();
  const bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable);

  if (already_placed) {
    return can_reuse_guarded_map_table(ep, node, target, data);
  }

  GuardedMapTable *guarded_map_table = build_guarded_map_table(ep, node, target, data);

  if (!guarded_map_table) {
    return false;
  }

  delete guarded_map_table;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse
