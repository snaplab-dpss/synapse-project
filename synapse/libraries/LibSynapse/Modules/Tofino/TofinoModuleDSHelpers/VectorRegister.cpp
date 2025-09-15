#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

VectorRegister *build_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  const std::vector<klee::ref<klee::Expr>> partitions = Register::partition_value(properties, data.value, ep->get_ctx().get_expr_structs());

  std::vector<bits_t> values_sizes;
  for (klee::ref<klee::Expr> partition : partitions) {
    bits_t partition_size = partition->getWidth();
    values_sizes.push_back(partition_size);
  }

  const DS_ID id = TofinoModuleFactory::build_vector_register_id(data.obj);

  VectorRegister *vector_register = new VectorRegister(properties, id, data.capacity, data.index->getWidth(), values_sizes);

  if (!tofino_ctx->can_place(ep, node, vector_register)) {
    delete vector_register;
    vector_register = nullptr;
  }

  return vector_register;
}

VectorRegister *get_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(data.obj);
  assert(!ds.empty() && "No vector registers found");
  assert(ds.size() == 1);
  DS *vr = *ds.begin();

  assert(vr->type == DSType::VectorRegister && "Unexpected type");
  return dynamic_cast<VectorRegister *>(vr);
}

} // namespace

std::string TofinoModuleFactory::build_vector_register_id(addr_t obj) { return "vector_register_" + std::to_string(obj); }

VectorRegister *TofinoModuleFactory::build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data) {
  VectorRegister *vector_register;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (already_placed) {
    vector_register = get_vector_register(ep, node, data);
  } else {
    vector_register = build_vector_register(ep, node, data);
  }

  return vector_register;
}

bool TofinoModuleFactory::can_build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data) {
  const Context &ctx       = ep->get_ctx();
  bool regs_already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    VectorRegister *vector_register = get_vector_register(ep, node, data);
    return vector_register != nullptr;
  }

  VectorRegister *vector_register = build_vector_register(ep, node, data);

  if (vector_register == nullptr) {
    return false;
  }

  delete vector_register;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse