#include <LibSynapse/Modules/Controller/Ignore.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
// We can ignore dchain_rejuvenate_index if the dchain is only used for
// linking a map with vectors. It doesn't even matter if the data structures
// are coalesced or not, we can freely ignore it regardless.
bool can_ignore_dchain_op(const Context &ctx, const call_t &call) {
  assert(call.function_name == "dchain_rejuvenate_index" && "Not a dchain call");

  klee::ref<klee::Expr> chain = call.args.at("chain").expr;
  const addr_t chain_addr     = expr_addr_to_obj_addr(chain);

  std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(chain_addr);

  if (!map_objs.has_value()) {
    return false;
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable) && !ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable)) {
    return false;
  }

  return true;
}

bool ds_ignore_logic(const Context &ctx, const Call *call_node) {
  const std::optional<addr_t> obj = get_obj_from_call(call_node);

  if (!obj.has_value()) {
    return false;
  }

  static std::unordered_map<DSImpl, std::unordered_set<std::string>> ds_ignore_logic_map = {
      {DSImpl::Tofino_FCFSCachedTable, {"dchain_rejuvenate_index", "dchain_free_index", "map_put"}},
      {DSImpl::Tofino_HeavyHitterTable, {"dchain_rejuvenate_index", "dchain_free_index"}},
  };

  const call_t &call = call_node->get_call();
  for (const auto &[ds_impl, functions] : ds_ignore_logic_map) {
    if (ctx.check_ds_impl(obj.value(), ds_impl)) {
      return functions.contains(call.function_name);
    }
  }

  return false;
}

bool should_ignore_coalesced_state_allocation(const Context &ctx, const Call *call_node) {
  using ignored_alloc_functions_t = std::unordered_set<std::string>;

  const std::unordered_map<DSImpl, ignored_alloc_functions_t> ignored_alloc_functions_per_ds{
      {DSImpl::Tofino_FCFSCachedTable, {"dchain_allocate"}},
      {DSImpl::Tofino_HeavyHitterTable, {"dchain_allocate"}},
  };

  const call_t &call             = call_node->get_call();
  klee::ref<klee::Expr> obj_expr = call_node->get_obj();

  if (obj_expr.isNull()) {
    return false;
  }

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  if (!ctx.has_ds_impl(obj)) {
    return false;
  }

  const DSImpl impl = ctx.get_ds_impl(obj);
  auto found_it     = ignored_alloc_functions_per_ds.find(impl);
  if (found_it == ignored_alloc_functions_per_ds.end()) {
    return false;
  }

  const ignored_alloc_functions_t &ignored_alloc_functions = found_it->second;
  return ignored_alloc_functions.find(call.function_name) != ignored_alloc_functions.end();
}

bool should_ignore(const Context &ctx, const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  const std::unordered_set<std::string> functions_to_always_ignore{
      "expire_items_single_map",
      "expire_items_single_map_iteratively",
      "cms_periodic_cleanup",
  };

  if (functions_to_always_ignore.find(call.function_name) != functions_to_always_ignore.end()) {
    return true;
  }

  if (call.function_name == "dchain_rejuvenate_index") {
    return can_ignore_dchain_op(ctx, call);
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return true;
  }

  if (call_node->is_vector_return_without_modifications()) {
    return true;
  }

  if (ds_ignore_logic(ctx, call_node)) {
    return true;
  }

  if (should_ignore_coalesced_state_allocation(ctx, call_node)) {
    return true;
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!should_ignore(ep->get_ctx(), node)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!should_ignore(ep->get_ctx(), node)) {
    return {};
  }

  Module *module  = new Ignore(node);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IgnoreFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!should_ignore(ctx, node)) {
    return {};
  }

  return std::make_unique<Ignore>(node);
}

} // namespace Controller
} // namespace LibSynapse