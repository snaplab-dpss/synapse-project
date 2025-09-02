#include <LibSynapse/Modules/Controller/DataplaneVectorTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct vector_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;
};

vector_table_data_t get_vector_table_data(const Call *call_node) {
  // We can implement even if we later update the vector's contents!

  const call_t &call = call_node->get_call();
  assert(call.function_name == "vector_borrow" && "Not a vector_borrow call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> cell             = call.extra_vars.at("borrowed_cell").second;

  const vector_table_data_t data = {
      .obj   = expr_addr_to_obj_addr(vector_addr_expr),
      .index = index,
      .value = cell,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (vector_borrow->is_vector_borrow_value_ignored()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(vector_borrow);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (vector_borrow->is_vector_borrow_value_ignored()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(vector_borrow);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  Module *module  = new DataplaneVectorTableLookup(ep->get_placement(node->get_id()), node, data.obj, data.index, data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (vector_borrow->is_vector_borrow_value_ignored()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(vector_borrow);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return std::make_unique<DataplaneVectorTableLookup>(get_type().instance_id, node, data.obj, data.index, data.value);
}

} // namespace Controller
} // namespace LibSynapse