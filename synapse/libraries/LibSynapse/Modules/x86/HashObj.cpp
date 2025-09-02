#include <LibSynapse/Modules/x86/HashObj.h>
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

  if (call.function_name != "hash_obj") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> HashObjFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> HashObjFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  const addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

  Module *module  = new HashObj(ep->get_placement(node->get_id()), node, obj_addr, size, hash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> HashObjFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  const addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

  return std::make_unique<HashObj>(get_type().instance_id, node, obj_addr, size, hash);
}

} // namespace x86
} // namespace LibSynapse