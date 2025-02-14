#include <LibSynapse/Modules/Controller/ParseHeader.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ParseHeaderFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParseHeaderFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return impls;
  }

  klee::ref<klee::Expr> chunk     = call.args.at("chunk").out;
  klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length    = call.args.at("length").expr;

  addr_t chunk_addr = LibCore::expr_addr_to_obj_addr(chunk);

  Module *module  = new ParseHeader(node, chunk_addr, out_chunk, length);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> ParseHeaderFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return {};
  }

  klee::ref<klee::Expr> chunk     = call.args.at("chunk").out;
  klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length    = call.args.at("length").expr;

  addr_t chunk_addr = LibCore::expr_addr_to_obj_addr(chunk);

  return std::make_unique<ParseHeader>(node, chunk_addr, out_chunk, length);
}

} // namespace Controller
} // namespace LibSynapse