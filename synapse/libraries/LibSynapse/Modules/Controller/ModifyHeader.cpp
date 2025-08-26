#include <LibSynapse/Modules/Controller/ModifyHeader.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::build_expr_mods;
using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_byte_swap_t;
using LibCore::expr_mod_t;
using LibCore::get_expr_byte_swaps;

std::optional<spec_impl_t> ModifyHeaderFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "packet_return_chunk") {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ModifyHeaderFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *packet_return_chunk = dynamic_cast<const Call *>(node);
  const call_t &call              = packet_return_chunk->get_call();

  if (call.function_name != "packet_return_chunk") {
    return {};
  }

  const Call *packet_borrow_chunk = packet_return_chunk->packet_borrow_from_return();
  assert(packet_borrow_chunk && "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  const addr_t hdr_addr                     = expr_addr_to_obj_addr(call.args.at("the_chunk").expr);
  klee::ref<klee::Expr> borrowed            = packet_borrow_chunk->get_call().extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned            = packet_return_chunk->get_call().args.at("the_chunk").in;
  const std::vector<expr_mod_t> changes     = build_expr_mods(borrowed, returned);
  const std::vector<expr_byte_swap_t> swaps = get_expr_byte_swaps(borrowed, returned);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  if (changes.empty()) {
    new_ep->process_leaf(node->get_next());
  } else {
    Module *module  = new ModifyHeader(type, node, hdr_addr, changes, swaps);
    EPNode *ep_node = new EPNode(module);

    const EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ModifyHeaderFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *packet_return_chunk = dynamic_cast<const Call *>(node);
  const call_t &call              = packet_return_chunk->get_call();

  if (call.function_name != "packet_return_chunk") {
    return {};
  }

  const Call *packet_borrow_chunk = packet_return_chunk->packet_borrow_from_return();
  assert(packet_borrow_chunk && "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  const addr_t hdr_addr                     = expr_addr_to_obj_addr(call.args.at("the_chunk").expr);
  klee::ref<klee::Expr> borrowed            = packet_borrow_chunk->get_call().extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned            = call.args.at("the_chunk").in;
  const std::vector<expr_mod_t> changes     = build_expr_mods(borrowed, returned);
  const std::vector<expr_byte_swap_t> swaps = get_expr_byte_swaps(borrowed, returned);

  if (changes.empty()) {
    return {};
  }

  return std::make_unique<ModifyHeader>(type, node, hdr_addr, changes, swaps);
}

} // namespace Controller
} // namespace LibSynapse