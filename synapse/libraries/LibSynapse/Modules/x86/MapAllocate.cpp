#include <LibSynapse/Modules/x86/MapAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>
#include <klee/Expr.h>
#include <klee/util/Ref.h>

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
  if (call.function_name != "map_allocate") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> MapAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map_out").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out  = call.args.at("map_out").out;
  symbol_t success               = call_node->get_local_symbol("map_allocation_succeeded");

  const addr_t map_addr = expr_addr_to_obj_addr(map_out);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  Module *module  = new MapAllocate(get_type().instance_id, node, capacity, key_size, map_out, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::x86_Map);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MapAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out  = call.args.at("map_out").out;
  symbol_t success               = call_node->get_local_symbol("map_allocation_succeeded");

  const addr_t map_addr = expr_addr_to_obj_addr(map_out);

  std::cerr << "MapAllocateFactory::create  addr=" << std::hex << map_addr << "  has=" << ctx.has_ds_impl(map_addr)
            << "  check=" << ctx.check_ds_impl(map_addr, DSImpl::x86_Map) << "  can=" << ctx.can_impl_ds(map_addr, DSImpl::x86_Map) << "\n";

  if (!ctx.check_ds_impl(map_addr, DSImpl::x86_Map)) {
    return {};
  }

  return std::make_unique<MapAllocate>(get_type().instance_id, node, capacity, key_size, map_out, success);
}

} // namespace x86
} // namespace LibSynapse
