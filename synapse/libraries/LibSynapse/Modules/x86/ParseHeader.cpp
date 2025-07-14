#include <LibSynapse/Modules/x86/ParseHeader.h>
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

  if (call.function_name != "packet_borrow_next_chunk") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ParseHeaderFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> ParseHeaderFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> chunk     = call.args.at("chunk").out;
  klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length    = call.args.at("length").expr;

  const addr_t chunk_addr = expr_addr_to_obj_addr(chunk);

  Module *module  = new ParseHeader(node, chunk_addr, out_chunk, length);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ParseHeaderFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> chunk     = call.args.at("chunk").out;
  klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length    = call.args.at("length").expr;

  const addr_t chunk_addr = expr_addr_to_obj_addr(chunk);

  return std::make_unique<ParseHeader>(node, chunk_addr, out_chunk, length);
}

} // namespace x86
} // namespace LibSynapse