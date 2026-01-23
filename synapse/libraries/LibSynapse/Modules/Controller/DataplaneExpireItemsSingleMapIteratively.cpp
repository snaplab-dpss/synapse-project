#include <LibSynapse/Modules/Controller/DataplaneExpireItemsSingleMapIteratively.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> DataplaneExpireItemsSingleMapIterativelyFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                      const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "expire_items_single_map_iteratively") {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneExpireItemsSingleMapIterativelyFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                  SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "expire_items_single_map_iteratively") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;

  const addr_t map_addr    = expr_addr_to_obj_addr(map_addr_expr);
  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  Module *module  = new DataplaneExpireItemsSingleMapIteratively(node, map_addr, vector_addr, start, n_elems);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneExpireItemsSingleMapIterativelyFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "expire_items_single_map_iteratively") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr    = call.args.at("map").expr;
  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> start            = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems          = call.args.at("n_elems").expr;

  const addr_t map_addr    = expr_addr_to_obj_addr(map_addr_expr);
  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  return std::make_unique<DataplaneExpireItemsSingleMapIteratively>(node, map_addr, vector_addr, start, n_elems);
}

} // namespace Controller
} // namespace LibSynapse