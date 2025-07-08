#include <LibSynapse/Modules/x86/HashObj.h>
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

  if (call.function_name != "hash_obj") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> HashObjFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> HashObjFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  const addr_t obj_addr = LibCore::expr_addr_to_obj_addr(obj_addr_expr);

  Module *module  = new HashObj(node, obj_addr, size, hash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> HashObjFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  addr_t obj_addr = LibCore::expr_addr_to_obj_addr(obj_addr_expr);

  return std::make_unique<HashObj>(node, obj_addr, size, hash);
}

} // namespace x86
} // namespace LibSynapse