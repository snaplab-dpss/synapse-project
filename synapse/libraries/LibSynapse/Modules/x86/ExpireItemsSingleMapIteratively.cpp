#include <LibSynapse/Modules/x86/ExpireItemsSingleMapIteratively.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "expire_items_single_map_iteratively") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ExpireItemsSingleMapIterativelyFactory::speculate(const EP *ep, const LibBDD::Node *node,
                                                                             const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> ExpireItemsSingleMapIterativelyFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                         LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;

  addr_t map_addr    = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  Module *module  = new ExpireItemsSingleMapIteratively(node, map_addr, vector_addr, start, n_elems);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> ExpireItemsSingleMapIterativelyFactory::create(const LibBDD::BDD *bdd, const Context &ctx,
                                                                       const LibBDD::Node *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;

  addr_t map_addr    = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  return std::make_unique<ExpireItemsSingleMapIteratively>(node, map_addr, vector_addr, start, n_elems);
}

} // namespace x86
} // namespace LibSynapse