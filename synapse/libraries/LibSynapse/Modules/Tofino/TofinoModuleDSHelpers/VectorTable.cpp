#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

VectorTable *build_vector_table(const EP *ep, const BDDNode *node, const TargetType type, const vector_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  assert(data.key->getWidth() == klee::Expr::Int32);
  const bits_t param_size = data.value->getWidth();

  const DS_ID id = "vector_table_" + std::to_string(data.obj);

  VectorTable *vector_table = new VectorTable(id, data.capacity, param_size);
  vector_table->add_table(node->get_id());

  if (!tofino_ctx->can_place(ep, node, vector_table)) {
    delete vector_table;
    vector_table = nullptr;
  }

  return vector_table;
}

VectorTable *get_vector_table(const EP *ep, const BDDNode *node, const TargetType type, const vector_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No vector table found");
  assert(ds.size() == 1);
  DS *vt = *ds.begin();

  assert(vt->type == DSType::VectorTable && "Unexpected type");
  return dynamic_cast<VectorTable *>(vt);
}

bool can_reuse_vector_table(const EP *ep, const BDDNode *node, const TargetType type, const vector_table_data_t &data) {
  VectorTable *vector_table = get_vector_table(ep, node, type, data);
  assert(vector_table && "Vector table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);
  assert(!vector_table->has_table(node->get_id()));

  VectorTable *clone = dynamic_cast<VectorTable *>(vector_table->clone());

  clone->add_table(node->get_id());
  vector_table = clone;

  bool can_place = tofino_ctx->can_place(ep, node, vector_table);
  delete vector_table;

  return can_place;
}

VectorTable *reuse_vector_table(const EP *ep, const BDDNode *node, const TargetType type, const vector_table_data_t &data) {
  VectorTable *vector_table = get_vector_table(ep, node, type, data);
  assert(vector_table && "Vector table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);
  assert(!vector_table->has_table(node->get_id()));

  vector_table->add_table(node->get_id());

  if (!tofino_ctx->can_place(ep, node, vector_table)) {
    return nullptr;
  }

  return vector_table;
}

} // namespace

VectorTable *TofinoModuleFactory::build_or_reuse_vector_table(const EP *ep, const BDDNode *node, const TargetType type,
                                                              const vector_table_data_t &data) {
  VectorTable *vector_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  if (already_placed) {
    vector_table = reuse_vector_table(ep, node, type, data);
  } else {
    vector_table = build_vector_table(ep, node, type, data);
  }

  return vector_table;
}

bool TofinoModuleFactory::can_build_or_reuse_vector_table(const EP *ep, const BDDNode *node, const TargetType type, const vector_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  if (already_placed) {
    return can_reuse_vector_table(ep, node, type, data);
  }

  VectorTable *vector_table = build_vector_table(ep, node, type, data);

  if (!vector_table) {
    return false;
  }

  delete vector_table;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse
