#include <LibSynapse/Modules/x86/ModifyHeader.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::build_expr_mods;
using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "packet_return_chunk") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ModifyHeaderFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> ModifyHeaderFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *packet_return_chunk = dynamic_cast<const Call *>(node);
  const call_t &call              = packet_return_chunk->get_call();

  if (call.function_name != "packet_return_chunk") {
    return {};
  }

  const Call *packet_borrow_chunk = packet_return_chunk->packet_borrow_from_return();
  assert(packet_borrow_chunk && "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("the_chunk").expr;
  klee::ref<klee::Expr> borrowed      = packet_borrow_chunk->get_call().extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned      = packet_return_chunk->get_call().args.at("the_chunk").in;
  std::cerr << "EP ID: " << ep->get_id() << "\n";
  if (borrowed->getWidth() != returned->getWidth()) {
    std::cerr << "Processing ModifyHeader at node " << node->dump(true) << "\n";
    std::cerr << "Borrowed: " << packet_borrow_chunk->dump(true) << "\n";
    std::cerr << "Returned: " << packet_return_chunk->dump(true) << "\n";
    std::cerr << "Borrowed expr: " << LibCore::expr_to_string(borrowed) << "\n";
    std::cerr << "Returned expr: " << LibCore::expr_to_string(returned) << "\n";
    BDDViz::visualize(ep->get_bdd(), true);
  }
  const std::vector<expr_mod_t> changes = build_expr_mods(borrowed, returned);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  if (changes.empty()) {
    new_ep->process_leaf(node->get_next());
  } else {
    Module *module  = new ModifyHeader(get_type().instance_id, node, hdr_addr_expr, changes);
    EPNode *ep_node = new EPNode(module);
    const EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ModifyHeaderFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *packet_return_chunk = dynamic_cast<const Call *>(node);
  const call_t &call              = packet_return_chunk->get_call();

  klee::ref<klee::Expr> hdr_addr_expr   = call.args.at("the_chunk").expr;
  klee::ref<klee::Expr> borrowed        = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned        = call.args.at("the_chunk").in;
  const std::vector<expr_mod_t> changes = build_expr_mods(borrowed, returned);

  if (changes.empty()) {
    return {};
  }

  return std::make_unique<ModifyHeader>(get_type().instance_id, node, hdr_addr_expr, changes);
}

} // namespace x86
} // namespace LibSynapse