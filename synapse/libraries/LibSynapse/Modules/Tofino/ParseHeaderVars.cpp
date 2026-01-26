#include <LibSynapse/Modules/Tofino/ParseHeaderVars.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {
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

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> code_path = call.args.at("code_path").expr;
  Symbols symbols                 = call_node->get_local_symbols();

  Module *module  = new ParseHeaderVars(get_type().instance_id, node, code_path, symbols);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

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

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> code_path = call.args.at("code_path").expr;
  Symbols symbols                 = call_node->get_local_symbols();

  return std::make_unique<ParseHeaderVars>(get_type().instance_id, node, code_path, symbols);
}

} // namespace Tofino
} // namespace LibSynapse
