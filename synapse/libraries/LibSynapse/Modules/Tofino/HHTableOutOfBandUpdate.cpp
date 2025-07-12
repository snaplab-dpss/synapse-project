#include <LibSynapse/Modules/Tofino/HHTableOutOfBandUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  hh_table_data_t(const Context &ctx, const LibBDD::Call *map_put) {
    const LibBDD::call_t &call = map_put->get_call();
    assert(call.function_name == "map_put" && "Not a map_put call");

    obj        = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key        = call.args.at("key").in;
    table_keys = Table::build_keys(key, ctx.get_expr_structs());
    value      = call.args.at("value").expr;
  }
};

const LibBDD::Call *get_future_map_put(const LibBDD::Node *node, addr_t map) {
  for (const LibBDD::Call *map_put : node->get_future_functions({"map_put"})) {
    const LibBDD::call_t &call     = map_put->get_call();
    klee::ref<klee::Expr> map_expr = call.args.at("map").expr;

    if (LibCore::expr_addr_to_obj_addr(map_expr) == map) {
      return map_put;
    }
  }

  return nullptr;
}

std::unique_ptr<LibBDD::BDD> rebuild_bdd(const EP *ep, const LibBDD::Node *node, const LibBDD::branch_direction_t &index_alloc_check,
                                         const LibBDD::map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
                                         const LibBDD::Node *&new_next_node) {
  const LibBDD::BDD *old_bdd       = ep->get_bdd();
  std::unique_ptr<LibBDD::BDD> bdd = std::make_unique<LibBDD::BDD>(*old_bdd);

  new_next_node = bdd->delete_branch(index_alloc_check.branch->get_id(), !index_alloc_check.direction);

  return bdd;
}

} // namespace

std::optional<spec_impl_t> HHTableOutOfBandUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return std::nullopt;
  }

  if (!ep->get_bdd()->is_index_alloc_on_unsuccessful_map_get(dchain_allocate_new_index)) {
    return std::nullopt;
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return std::nullopt;
  }

  const LibBDD::Call *map_put = get_future_map_put(node, map_objs.map);
  assert(map_put && "map_put not found");

  const hh_table_data_t table_data(ctx, map_put);

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  spec_impl_t spec_impl(decide(ep, node), ctx);

  // Get all nodes executed on a successful index allocation.
  const LibBDD::branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();
  assert(index_alloc_check.branch && "Branch checking index allocation not found");

  const hit_rate_t index_alloc_failure_hr = ep->get_ctx().get_profiler().get_hr(index_alloc_check.get_failure_node());
  if (index_alloc_failure_hr == 0_hr) {
    return std::nullopt;
  }

  const LibBDD::Node *on_hh = index_alloc_check.direction ? index_alloc_check.branch->get_on_true() : index_alloc_check.branch->get_on_false();
  std::vector<const LibBDD::Call *> targets = on_hh->get_coalescing_nodes_from_key(table_data.key, map_objs);

  // Ignore all coalescing nodes if the index allocation is successful (i.e. it is a heavy hitter).
  for (const LibBDD::Node *tgt : targets) {
    spec_impl.skip.insert(tgt->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> HHTableOutOfBandUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return {};
  }

  if (!ep->get_bdd()->is_index_alloc_on_unsuccessful_map_get(dchain_allocate_new_index)) {
    return {};
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return {};
  }

  const LibBDD::branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();
  assert(index_alloc_check.branch && "Branch checking index allocation not found");

  const hit_rate_t index_alloc_failure_hr = ep->get_ctx().get_profiler().get_hr(index_alloc_check.get_failure_node());
  if (index_alloc_failure_hr == 0_hr) {
    return {};
  }

  bool logic_before_dchain_check = false;
  if (dchain_allocate_new_index->get_next() != index_alloc_check.branch) {
    logic_before_dchain_check = true;
  }

  // We require the HH table to be implemented by the HH Table Read module.
  // We actually don't really need this, we could query the CMS right here, but we leave it like this for now.

  if (!ep->get_ctx().check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const LibBDD::Call *map_put = get_future_map_put(node, map_objs.map);
  assert(map_put && "map_put not found");

  const hh_table_data_t table_data(ep->get_ctx(), map_put);
  const HHTable *hh_table = get_tofino_ctx(ep)->get_data_structures().get_single_ds<HHTable>(table_data.obj);

  Module *module  = new HHTableOutOfBandUpdate(node, hh_table->id, table_data.obj);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const LibBDD::Node *node_replacing_index_check;
  std::unique_ptr<LibBDD::BDD> new_bdd = rebuild_bdd(new_ep.get(), node, index_alloc_check, map_objs, table_data.key, node_replacing_index_check);

  const LibBDD::Node *new_next_node = node_replacing_index_check;
  if (logic_before_dchain_check) {
    new_next_node = new_bdd->get_node_by_id(dchain_allocate_new_index->get_next()->get_id());
    assert(new_next_node && "Next node not found");
  }

  const std::vector<klee::ref<klee::Expr>> on_succesful_allocation = index_alloc_check.get_success_node()->get_ordered_branch_constraints();

  new_ep->get_mutable_ctx().get_mutable_profiler().remove(on_succesful_allocation);

  EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> HHTableOutOfBandUpdateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  return {};
}

} // namespace Tofino
} // namespace LibSynapse