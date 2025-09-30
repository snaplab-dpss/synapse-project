#include <LibSynapse/Modules/x86/VectorAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_allocate") {
    return false;
  }

  return true;
}
} // namespace
std::optional<spec_impl_t> VectorAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;
  symbol_t success                 = call_node->get_local_symbol("vector_alloc_success");

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_out);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;
  symbol_t success                 = call_node->get_local_symbol("vector_alloc_success");
  const addr_t vector_addr         = expr_addr_to_obj_addr(vector_out);

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  Module *module  = new VectorAllocate(get_type().instance_id, node, elem_size, capacity, vector_out, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::x86_Vector);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;
  symbol_t success                 = call_node->get_local_symbol("vector_alloc_success");

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_out);

  std::cerr << "VectorAllocate::create  addr=" << std::hex << vector_addr << "  has=" << ctx.has_ds_impl(vector_addr)
            << "  check=" << ctx.check_ds_impl(vector_addr, DSImpl::x86_Vector) << "  can=" << ctx.can_impl_ds(vector_addr, DSImpl::x86_Vector)
            << "\n";

  if (!ctx.check_ds_impl(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  return std::make_unique<VectorAllocate>(get_type().instance_id, node, elem_size, capacity, vector_out, success);
}

} // namespace x86
} // namespace LibSynapse
