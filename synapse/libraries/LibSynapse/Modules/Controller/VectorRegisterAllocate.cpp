#include <LibSynapse/Modules/Controller/VectorRegisterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> VectorRegisterAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_out);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorRegisterAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return impls;
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_out);

  if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  Module *module  = new VectorRegisterAllocate(node, vector_addr, elem_size, capacity);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> VectorRegisterAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_out);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return std::make_unique<VectorRegisterAllocate>(node, vector_addr, elem_size, capacity);
}

} // namespace Controller
} // namespace LibSynapse