#include <LibSynapse/Modules/Controller/Ignore.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Controller {

namespace {
// We can ignore dchain_rejuvenate_index if the dchain is only used for
// linking a map with vectors. It doesn't even matter if the data structures
// are coalesced or not, we can freely ignore it regardless.
bool can_ignore_dchain_op(const Context &ctx, const LibBDD::call_t &call) {
  assert(call.function_name == "dchain_rejuvenate_index" && "Not a dchain call");

  klee::ref<klee::Expr> chain = call.args.at("chain").expr;
  addr_t chain_addr           = LibCore::expr_addr_to_obj_addr(chain);

  std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(chain_addr);

  if (!map_objs.has_value()) {
    return false;
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable) && !ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable)) {
    return false;
  }

  return true;
}

bool ds_ignore_logic(const Context &ctx, const LibBDD::call_t &call) {
  addr_t obj;

  if (call.function_name == "dchain_rejuvenate_index" || call.function_name == "dchain_allocate_new_index" ||
      call.function_name == "dchain_free_index") {
    obj = LibCore::expr_addr_to_obj_addr(call.args.at("chain").expr);
  } else if (call.function_name == "vector_borrow" || call.function_name == "vector_return") {
    obj = LibCore::expr_addr_to_obj_addr(call.args.at("vector").expr);
  } else {
    return false;
  }

  if (ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable) || ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return true;
  }

  return false;
}

bool should_ignore_coalesced_state_allocation(const Context &ctx, const LibBDD::Call *call_node) {
  using ignored_alloc_functions_t = std::unordered_set<std::string>;

  const std::unordered_map<DSImpl, ignored_alloc_functions_t> ignored_alloc_functions_per_ds{
      {DSImpl::Tofino_FCFSCachedTable, {"dchain_allocate"}},
      {DSImpl::Tofino_HeavyHitterTable, {"dchain_allocate"}},
  };

  const LibBDD::call_t &call     = call_node->get_call();
  klee::ref<klee::Expr> obj_expr = call_node->get_obj();

  if (obj_expr.isNull()) {
    return false;
  }

  const addr_t obj = LibCore::expr_addr_to_obj_addr(obj_expr);

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

bool should_ignore(const Context &ctx, const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

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

  if (ds_ignore_logic(ctx, call)) {
    return true;
  }

  if (should_ignore_coalesced_state_allocation(ctx, call_node)) {
    return true;
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!should_ignore(ep->get_ctx(), node)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
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

std::unique_ptr<Module> IgnoreFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!should_ignore(ctx, node)) {
    return {};
  }

  return std::make_unique<Ignore>(node);
}

} // namespace Controller
} // namespace LibSynapse