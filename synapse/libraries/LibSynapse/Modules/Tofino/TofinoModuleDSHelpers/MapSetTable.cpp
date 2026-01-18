#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

MapSetTable *build_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  bits_t key_size = 0;
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    key_size += key->getWidth();
    keys_size.push_back(key->getWidth());
  }

  assert(data.value->getWidth() == klee::Expr::Int32);

  const DS_ID id = "map_set_table_" + std::to_string(data.obj);

  MapSetTable *map_set_table = new MapSetTable(id, data.capacity, key_size);
  map_set_table->add_table(node->get_id(), keys_size);

  if (!tofino_ctx->can_place(ep, node, map_set_table)) {
    delete map_set_table;
    map_set_table = nullptr;
  }

  return map_set_table;
}

MapSetTable *get_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No map table found");
  assert(ds.size() == 1);
  DS *mt = *ds.begin();

  assert(mt->type == DSType::MapSetTable && "Unexpected type");
  return dynamic_cast<MapSetTable *>(mt);
}

bool can_reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  MapSetTable *map_set_table = get_map_set_table(ep, node, data);
  assert(map_set_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_set_table->has_table(node->get_id()));

  MapSetTable *clone = dynamic_cast<MapSetTable *>(map_set_table->clone());

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  clone->add_table(node->get_id(), keys_size);
  map_set_table = clone;

  bool can_place = tofino_ctx->can_place(ep, node, map_set_table);
  delete map_set_table;

  return can_place;
}

MapSetTable *reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  MapSetTable *map_set_table = get_map_set_table(ep, node, data);
  assert(map_set_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_set_table->has_table(node->get_id()));

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  map_set_table->add_table(node->get_id(), keys_size);

  if (!tofino_ctx->can_place(ep, node, map_set_table)) {
    map_set_table->remove_table(node->get_id());
    return nullptr;
  }

  return map_set_table;
}

} // namespace

MapSetTable *TofinoModuleFactory::build_or_reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  MapSetTable *map_set_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable);

  if (already_placed) {
    map_set_table = reuse_map_set_table(ep, node, data);
  } else {
    map_set_table = build_map_set_table(ep, node, data);
  }

  return map_set_table;
}

bool TofinoModuleFactory::can_build_or_reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable);

  if (already_placed) {
    return can_reuse_map_set_table(ep, node, data);
  }

  MapSetTable *map_set_table = build_map_set_table(ep, node, data);

  if (!map_set_table) {
    return false;
  }

  delete map_set_table;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse