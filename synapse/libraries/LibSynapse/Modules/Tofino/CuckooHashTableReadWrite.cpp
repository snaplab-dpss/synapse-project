#include <LibSynapse/Modules/Tofino/CuckooHashTableReadWrite.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>

namespace LibSynapse {
namespace Tofino {
namespace {

using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

struct cuckoo_hash_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  u32 capacity;

  cuckoo_hash_table_data_t(const Context &ctx, const Call *map_get, std::vector<const Call *> future_map_puts) {
    assert(!future_map_puts.empty() && "No future map puts found");
    const Call *map_put = future_map_puts.front();

    const call_t &get_call = map_get->get_call();
    const call_t &put_call = map_put->get_call();

    obj              = expr_addr_to_obj_addr(get_call.args.at("map").expr);
    key              = get_call.args.at("key").in;
    read_value       = get_call.args.at("value_out").out;
    write_value      = put_call.args.at("value").expr;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    capacity         = ctx.get_map_config(obj).capacity;
  }
};

bool update_map_get_success_hit_rate(const EP *ep, Context &ctx, const BDDNode *map_get, addr_t map, klee::ref<klee::Expr> key, u32 capacity,
                                     const branch_direction_t &mgsc) {
  const hit_rate_t success_rate = TofinoModuleFactory::get_cuckoo_hash_table_hit_success_rate(ep, ctx, mgsc.branch, map, key, capacity);

  assert(mgsc.branch && "No branch checking map_get success");
  const BDDNode *on_success = mgsc.direction ? mgsc.branch->get_on_true() : mgsc.branch->get_on_false();
  const BDDNode *on_failure = mgsc.direction ? mgsc.branch->get_on_false() : mgsc.branch->get_on_true();

  const hit_rate_t branch_hr      = ctx.get_profiler().get_hr(mgsc.branch);
  const hit_rate_t new_success_hr = hit_rate_t{branch_hr * success_rate};
  const hit_rate_t new_failure_hr = hit_rate_t{branch_hr * (1 - success_rate)};

  if (!ctx.get_profiler().can_set(on_success->get_ordered_branch_constraints()) ||
      !ctx.get_profiler().can_set(on_failure->get_ordered_branch_constraints())) {
    return false;
  }

  ctx.get_mutable_profiler().set(on_success->get_ordered_branch_constraints(), new_success_hr);
  ctx.get_mutable_profiler().set(on_failure->get_ordered_branch_constraints(), new_failure_hr);

  return true;
}

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const BDDNode *on_success, const map_coalescing_objs_t &map_objs,
                                                               klee::ref<klee::Expr> key) {
  const std::vector<const Call *> coalescing_nodes = on_success->get_coalescing_nodes_from_key(key, map_objs);

  std::vector<const BDDNode *> nodes_to_ignore;
  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);

    const call_t &call = coalescing_node->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      const branch_direction_t index_alloc_check = ep->get_bdd()->find_branch_checking_index_alloc(coalescing_node);

      // FIXME: We ignore all logic happening when the index is not
      // successfuly allocated. This is a major simplification, as the NF
      // might be doing something useful here. We never encountered a scenario
      // where this was the case, but it could happen nonetheless.
      if (index_alloc_check.branch) {
        nodes_to_ignore.push_back(index_alloc_check.branch);

        const BDDNode *next = index_alloc_check.direction ? index_alloc_check.branch->get_on_false() : index_alloc_check.branch->get_on_true();

        next->visit_nodes([&nodes_to_ignore](const BDDNode *node) {
          nodes_to_ignore.push_back(node);
          return BDDNodeVisitAction::Continue;
        });
      }
    }
  }

  return nodes_to_ignore;
}

} // namespace

std::optional<spec_impl_t> CuckooHashTableReadWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    return {};
  }

  const cuckoo_hash_table_data_t cuckoo_hash_table_data(ep->get_ctx(), map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cuckoo_hash_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (map_objs->vectors.size() != 1) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_CuckooHashTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_CuckooHashTable) ||
      !ctx.can_impl_ds(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable)) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  if (!can_build_or_reuse_cuckoo_hash_table(ep, node, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key, cuckoo_hash_table_data.capacity)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_CuckooHashTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_CuckooHashTable);
  new_ctx.save_ds_impl(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable);

  if (!update_map_get_success_hit_rate(ep, new_ctx, map_get, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key, cuckoo_hash_table_data.capacity,
                                       mpsc)) {
    return {};
  }

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  std::vector<const BDDNode *> ignore_nodes = get_nodes_to_speculatively_ignore(ep, map_get, map_objs.value(), cuckoo_hash_table_data.key);
  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> CuckooHashTableReadWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    return {};
  }

  const cuckoo_hash_table_data_t cuckoo_hash_table_data(ep->get_ctx(), map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(cuckoo_hash_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_CuckooHashTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_CuckooHashTable) ||
      !ep->get_ctx().can_impl_ds(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable)) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  CuckooHashTable *cuckoo_hash_table =
      build_or_reuse_cuckoo_hash_table(ep, node, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key, cuckoo_hash_table_data.capacity);
  if (!cuckoo_hash_table) {
    return {};
  }

  Module *module  = new CuckooHashTableReadWrite(node, cuckoo_hash_table->id, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key,
                                                 cuckoo_hash_table_data.read_value, cuckoo_hash_table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  if (!update_map_get_success_hit_rate(new_ep.get(), new_ep->get_mutable_ctx(), map_get, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key,
                                       cuckoo_hash_table_data.capacity, mpsc)) {
    delete ep_node;
    delete cuckoo_hash_table;
    return {};
  }

  new_ep->get_mutable_ctx().save_ds_impl(map_objs->map, DSImpl::Tofino_CuckooHashTable);
  new_ep->get_mutable_ctx().save_ds_impl(map_objs->dchain, DSImpl::Tofino_CuckooHashTable);
  new_ep->get_mutable_ctx().save_ds_impl(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, map_objs->map, cuckoo_hash_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CuckooHashTableReadWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    return {};
  }

  const cuckoo_hash_table_data_t cuckoo_hash_table_data(ctx, map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cuckoo_hash_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_CuckooHashTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_CuckooHashTable)) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const CuckooHashTable *cuckoo_hash_table = dynamic_cast<const CuckooHashTable *>(*ds.begin());

  return std::make_unique<CuckooHashTableReadWrite>(node, cuckoo_hash_table->id, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key,
                                                    cuckoo_hash_table_data.read_value, cuckoo_hash_table_data.map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse