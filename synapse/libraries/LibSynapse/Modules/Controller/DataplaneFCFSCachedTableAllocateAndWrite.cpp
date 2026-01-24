#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableAllocateAndWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS_ID;
using Tofino::Table;

namespace {

DS_ID get_fcfs_ct_id(const Context &ctx, addr_t obj) {
  const Tofino::TofinoContext *tofino_ctx                 = ctx.get_target_ctx<Tofino::TofinoContext>();
  const std::unordered_set<Tofino::DS *> &data_structures = tofino_ctx->get_data_structures().get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  const Tofino::DS *ds = *data_structures.begin();
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Not a FCFS cached table");
  return ds->id;
}

struct allocate_and_write_pattern_t {
  const Call *dchain_allocate_new_index;
  std::vector<const Call *> map_puts;
  const BDDNode *on_write_success;
  const BDDNode *on_write_failure;
  symbol_t new_index;
  symbol_t index_allocation_success;
};

bool is_allocate_and_write(const Context &ctx, const BDD *bdd, const BDDNode *node, allocate_and_write_pattern_t &pattern) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  const branch_direction_t index_alloc_success_direction = bdd->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (index_alloc_success_direction.branch == nullptr) {
    return false;
  }

  const BDDNode *on_index_allocation_success = index_alloc_success_direction.get_success_node();
  const BDDNode *on_index_allocation_failure = index_alloc_success_direction.get_failure_node();

  if (on_index_allocation_success->get_type() != BDDNodeType::Call) {
    return false;
  }

  const addr_t obj = expr_addr_to_obj_addr(dchain_allocate_new_index->get_call().args.at("chain").expr);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return false;
  }

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, map_objs.value(), future_map_puts)) {
    return false;
  }

  const BDDNode *on_write_success = on_index_allocation_success->get_next();

  pattern.map_puts                  = future_map_puts;
  pattern.dchain_allocate_new_index = dchain_allocate_new_index;
  pattern.on_write_success          = on_write_success;
  pattern.on_write_failure          = on_index_allocation_failure;
  pattern.new_index                 = dchain_allocate_new_index->get_local_symbol("new_index");
  pattern.index_allocation_success  = dchain_allocate_new_index->get_local_symbol("not_out_of_space");

  return true;
}

struct fcfs_ct_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_ct_data_t> build_fcfs_ct_data(const Context &ctx, const allocate_and_write_pattern_t &pattern) {
  const call_t &put_call = pattern.map_puts.at(0)->get_call();

  fcfs_ct_data_t data;
  data.obj         = expr_addr_to_obj_addr(put_call.args.at("map").expr);
  data.key         = put_call.args.at("key").in;
  data.write_value = put_call.args.at("value").expr;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }
  data.map_objs = map_objs.value();

  return data;
}

std::unique_ptr<BDD> rebuild_bdd(EP *new_ep, const BDDNode *node, const allocate_and_write_pattern_t &pattern, const fcfs_ct_data_t &fcfs_ct_data,
                                 const BDDNode *&new_next_node) {
  const BDD *old_bdd       = new_ep->get_bdd();
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(*old_bdd);

  new_next_node = bdd->get_node_by_id(node->get_next()->get_id());

  for (const Call *map_put : pattern.map_puts) {
    const BDDNode *new_node = bdd->delete_non_branch(map_put->get_id());
    if (map_put->get_id() == new_next_node->get_id()) {
      new_next_node = new_node;
    }
  }

  return bdd;
}

} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableAllocateAndWriteFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                      const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  allocate_and_write_pattern_t pattern;
  if (!is_allocate_and_write(speculations.ctx, ep->get_bdd(), node, pattern)) {
    return {};
  }

  std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(speculations.ctx, pattern);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!speculations.ctx.check_ds_impl(fcfs_ct_data->obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  spec_impl_t spec_impl(decide(ep, node), speculations.ctx);
  for (const Call *map_put : pattern.map_puts) {
    spec_impl.skip.insert(map_put->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> DataplaneFCFSCachedTableAllocateAndWriteFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                  SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  allocate_and_write_pattern_t pattern;
  if (!is_allocate_and_write(ep->get_ctx(), ep->get_bdd(), node, pattern)) {
    return {};
  }

  std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(ep->get_ctx(), pattern);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!ep->get_ctx().check_ds_impl(fcfs_ct_data->obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const DS_ID id = get_fcfs_ct_id(ep->get_ctx(), fcfs_ct_data->obj);

  Module *module  = new DataplaneFCFSCachedTableAllocateAndWrite(node, id, fcfs_ct_data->obj, fcfs_ct_data->key, fcfs_ct_data->write_value,
                                                                 pattern.index_allocation_success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const BDDNode *next_node     = node->get_next();
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, pattern, fcfs_ct_data.value(), next_node);

  const EPLeaf leaf(ep_node, next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableAllocateAndWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibSynapse