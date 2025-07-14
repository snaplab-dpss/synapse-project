#include <LibSynapse/Modules/x86/MapErase.h>
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

  if (call.function_name != "map_erase") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> MapEraseFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapEraseFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> trash         = call.args.at("trash").out;

  const addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  Module *module  = new MapErase(node, map_addr, key, trash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::x86_Map);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MapEraseFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> trash         = call.args.at("trash").out;

  const addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.check_ds_impl(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  return std::make_unique<MapErase>(node, map_addr, key, trash);
}

} // namespace x86
} // namespace LibSynapse