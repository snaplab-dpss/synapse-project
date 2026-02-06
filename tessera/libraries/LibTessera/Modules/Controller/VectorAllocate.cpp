#include <LibTessera/Modules/Controller/VectorAllocate.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> VectorAllocateFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
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

  if (!speculations.ctx.can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> VectorAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
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

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  Module *module  = new VectorAllocate(node, vector_addr, elem_size, capacity);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), vector_addr, DSImpl::Controller_Vector);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
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

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  return std::make_unique<VectorAllocate>(node, vector_addr, elem_size, capacity);
}

} // namespace Controller
} // namespace LibTessera