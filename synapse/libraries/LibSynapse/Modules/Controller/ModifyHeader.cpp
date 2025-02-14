#include <LibSynapse/Modules/Controller/ModifyHeader.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ModifyHeaderFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_return_chunk") {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ModifyHeaderFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                      LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *packet_return_chunk = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call              = packet_return_chunk->get_call();

  if (call.function_name != "packet_return_chunk") {
    return impls;
  }

  const LibBDD::Call *packet_borrow_chunk = ep->packet_borrow_from_return(packet_return_chunk);
  assert(packet_borrow_chunk && "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  addr_t hdr_addr                          = LibCore::expr_addr_to_obj_addr(call.args.at("the_chunk").expr);
  klee::ref<klee::Expr> borrowed           = packet_borrow_chunk->get_call().extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned           = packet_return_chunk->get_call().args.at("the_chunk").in;
  std::vector<LibCore::expr_mod_t> changes = LibCore::build_expr_mods(borrowed, returned);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  if (changes.empty()) {
    new_ep->process_leaf(node->get_next());
    return impls;
  }

  Module *module  = new ModifyHeader(node, hdr_addr, changes);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> ModifyHeaderFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_return_chunk") {
    return {};
  }

  const LibBDD::Call *packet_borrow_chunk = call_node->packet_borrow_from_return();
  assert(packet_borrow_chunk && "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  addr_t hdr_addr                          = LibCore::expr_addr_to_obj_addr(call.args.at("the_chunk").expr);
  klee::ref<klee::Expr> borrowed           = packet_borrow_chunk->get_call().extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned           = call.args.at("the_chunk").in;
  std::vector<LibCore::expr_mod_t> changes = LibCore::build_expr_mods(borrowed, returned);

  if (changes.empty()) {
    return {};
  }

  return std::make_unique<ModifyHeader>(node, hdr_addr, changes);
}

} // namespace Controller
} // namespace LibSynapse