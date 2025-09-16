#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

MapTable *build_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  bits_t key_size = 0;
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    key_size += key->getWidth();
    keys_size.push_back(key->getWidth());
  }

  assert(data.value->getWidth() == klee::Expr::Int32);

  const DS_ID id = "map_table_" + std::to_string(data.obj);

  MapTable *map_table = new MapTable(id, data.capacity, key_size);
  map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->can_place(ep, node, map_table)) {
    delete map_table;
    map_table = nullptr;
  }

  return map_table;
}

MapTable *get_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No map table found");
  assert(ds.size() == 1);
  DS *mt = *ds.begin();

  assert(mt->type == DSType::MapTable && "Unexpected type");
  return dynamic_cast<MapTable *>(mt);
}

bool can_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  MapTable *map_table = get_map_table(ep, node, data);
  assert(map_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_table->has_table(node->get_id()));

  MapTable *clone = dynamic_cast<MapTable *>(map_table->clone());

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  clone->add_table(node->get_id(), keys_size, data.time_aware);
  map_table = clone;

  bool can_place = tofino_ctx->can_place(ep, node, map_table);
  delete map_table;

  return can_place;
}

MapTable *reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  MapTable *map_table = get_map_table(ep, node, data);
  assert(map_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_table->has_table(node->get_id()));

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->can_place(ep, node, map_table)) {
    return nullptr;
  }

  return map_table;
}

} // namespace

MapTable *TofinoModuleFactory::build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  MapTable *map_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  if (already_placed) {
    map_table = reuse_map_table(ep, node, data);
  } else {
    map_table = build_map_table(ep, node, data);
  }

  return map_table;
}

bool TofinoModuleFactory::can_build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  if (already_placed) {
    return can_reuse_map_table(ep, node, data);
  }

  MapTable *map_table = build_map_table(ep, node, data);

  if (!map_table) {
    return false;
  }

  delete map_table;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse