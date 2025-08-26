#include <LibSynapse/Modules/Controller/DataplaneVectorTableUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct vector_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

vector_table_data_t get_vector_table_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "vector_return" && "Not a vector_return call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;

  const vector_table_data_t data = {
      .obj   = expr_addr_to_obj_addr(vector_addr_expr),
      .key   = index,
      .value = value,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorTableUpdateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (call_node->is_vector_return_without_modifications()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorTableUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (call_node->is_vector_return_without_modifications()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  Module *module  = new DataplaneVectorTableUpdate(type, node, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorTableUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (call_node->is_vector_return_without_modifications()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return std::make_unique<DataplaneVectorTableUpdate>(type, node, data.obj, data.key, data.value);
}

} // namespace Controller
} // namespace LibSynapse