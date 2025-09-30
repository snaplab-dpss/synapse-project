#include <LibSynapse/Modules/x86/ExpireItemsSingleMapIteratively.h>
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

  if (call.function_name != "expire_items_single_map_iteratively") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ExpireItemsSingleMapIterativelyFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> ExpireItemsSingleMapIterativelyFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;
  klee::ref<klee::Expr> n_freed_flows    = call_node->get_local_symbol("number_of_freed_flows").expr;

  Module *module  = new ExpireItemsSingleMapIteratively(get_type().instance_id, node, vector_addr_expr, map_addr_expr, start, n_elems, n_freed_flows);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ExpireItemsSingleMapIterativelyFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;
  klee::ref<klee::Expr> n_freed_flows    = call_node->get_local_symbol("number_of_freed_flows").expr;

  return std::make_unique<ExpireItemsSingleMapIteratively>(get_type().instance_id, node, vector_addr_expr, map_addr_expr, start, n_elems,
                                                           n_freed_flows);
}

} // namespace x86
} // namespace LibSynapse