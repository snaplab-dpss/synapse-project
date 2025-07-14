#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> DataplaneVectorRegisterAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_out);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorRegisterAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_out);

  if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  Module *module  = new DataplaneVectorRegisterAllocate(node, vector_addr, elem_size, capacity);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorRegisterAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_out);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return std::make_unique<DataplaneVectorRegisterAllocate>(node, vector_addr, elem_size, capacity);
}

} // namespace Controller
} // namespace LibSynapse