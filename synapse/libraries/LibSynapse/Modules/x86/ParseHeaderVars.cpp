#include <LibSynapse/Modules/x86/ParseHeaderVars.h>
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

  if (call.function_name != "packet_parse_header_vars") {
    return false;
  }

  return true;
}

/*std::unique_ptr<BDD> replicate_hdr_parsing_ops(const EP *ep, const BDDNode *node, const BDDNode *&next) {

  bdd_node_ids_t s2d_ids      = node->get_prev_s2d_node_id();
  bdd_node_ids_t target_roots = ep->get_target_roots(ep->get_active_target());
  bdd_node_ids_t stop_nodes   = target_roots;

  stop_nodes.insert(s2d_ids.begin(), s2d_ids.end());

  std::list<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk"}, stop_nodes);
  std::list<const Call *> prev_returns = node->get_prev_functions({"packet_return_chunk"}, stop_nodes);

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);
  const BDDNode *new_next      = new_bdd->add_cloned_non_branches(next->get_id(), hdr_parsing_ops);

  if (new_next) {
    next = new_bdd->get_node_by_id(new_next->get_id());
  }

  return new_bdd;
}*/
} // namespace

std::optional<spec_impl_t> ParseHeaderVarsFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> ParseHeaderVarsFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  Symbols symbols            = get_relevant_dataplane_state(ep, node, get_target());
  symbols.remove("packet_chunks");
  symbols.remove("DEVICE");

  Module *module  = new ParseHeaderVars(get_type().instance_id, node, symbols);
  EPNode *ep_node = new EPNode(module);

  // std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ParseHeaderVarsFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  return {};
}

} // namespace x86
} // namespace LibSynapse