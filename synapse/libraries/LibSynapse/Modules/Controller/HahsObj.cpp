#include <LibSynapse/Modules/Controller/HashObj.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> HashObjFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> HashObjFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  const addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

  Module *module  = new HashObj(node, obj_addr, size, hash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> HashObjFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size          = call.args.at("size").expr;
  klee::ref<klee::Expr> hash          = call.ret;

  const addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

  return std::make_unique<HashObj>(node, obj_addr, size, hash);
}

} // namespace Controller
} // namespace LibSynapse