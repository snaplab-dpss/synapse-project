#include <LibSynapse/Modules/x86/ExpireItemsSingleMap.h>
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

  if (call.function_name != "expire_items_single_map") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ExpireItemsSingleMapFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> ExpireItemsSingleMapFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain      = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector      = call.args.at("vector").expr;
  klee::ref<klee::Expr> map         = call.args.at("map").expr;
  klee::ref<klee::Expr> time        = call.args.at("time").expr;
  klee::ref<klee::Expr> total_freed = call.ret;

  const addr_t map_addr    = LibCore::expr_addr_to_obj_addr(map);
  const addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector);
  const addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain);

  Module *module  = new ExpireItemsSingleMap(node, dchain_addr, vector_addr, map_addr, time, total_freed);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ExpireItemsSingleMapFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain      = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector      = call.args.at("vector").expr;
  klee::ref<klee::Expr> map         = call.args.at("map").expr;
  klee::ref<klee::Expr> time        = call.args.at("time").expr;
  klee::ref<klee::Expr> total_freed = call.ret;

  addr_t map_addr    = LibCore::expr_addr_to_obj_addr(map);
  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector);
  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain);

  return std::make_unique<ExpireItemsSingleMap>(node, dchain_addr, vector_addr, map_addr, time, total_freed);
}

} // namespace x86
} // namespace LibSynapse