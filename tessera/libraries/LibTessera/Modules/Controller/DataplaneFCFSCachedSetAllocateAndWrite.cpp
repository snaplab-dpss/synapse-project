#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetAllocateAndWrite.h>
#include <LibTessera/Modules/Tofino/FCFSCachedSetReadInsert.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibTessera::Tofino::FCFSCachedSetReadInsert;

using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

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

struct fcfs_cs_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_cs_data_t> build_fcfs_cs_data(const Context &ctx, const allocate_and_write_pattern_t &pattern) {
  const call_t &put_call = pattern.map_puts.at(0)->get_call();

  fcfs_cs_data_t data;
  data.obj = expr_addr_to_obj_addr(put_call.args.at("map").expr);
  data.key = put_call.args.at("key").in;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }
  data.map_objs = map_objs.value();

  return data;
}

std::unique_ptr<BDD> rebuild_bdd(EP *new_ep, const BDDNode *node, const allocate_and_write_pattern_t &pattern, const fcfs_cs_data_t &fcfs_cs_data,
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

std::optional<spec_impl_t> DataplaneFCFSCachedSetAllocateAndWriteFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                    const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  allocate_and_write_pattern_t pattern;
  if (!is_allocate_and_write(speculations.ctx, ep->get_bdd(), node, pattern)) {
    return {};
  }

  std::optional<fcfs_cs_data_t> fcfs_cs_data = build_fcfs_cs_data(speculations.ctx, pattern);
  if (!fcfs_cs_data.has_value()) {
    return {};
  }

  if (!speculations.ctx.check_ds_impl(fcfs_cs_data->obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  spec_impl_t spec_impl(decide(ep, node), speculations.ctx);
  for (const Call *map_put : pattern.map_puts) {
    spec_impl.skip.insert(map_put->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> DataplaneFCFSCachedSetAllocateAndWriteFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  allocate_and_write_pattern_t pattern;
  if (!is_allocate_and_write(ep->get_ctx(), ep->get_bdd(), node, pattern)) {
    return {};
  }

  std::optional<fcfs_cs_data_t> fcfs_cs_data = build_fcfs_cs_data(ep->get_ctx(), pattern);
  if (!fcfs_cs_data.has_value()) {
    return {};
  }

  if (!ep->get_ctx().check_ds_impl(fcfs_cs_data->obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  Module *module  = new DataplaneFCFSCachedSetAllocateAndWrite(node, fcfs_cs_data->obj, fcfs_cs_data->key, pattern.index_allocation_success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const BDDNode *next_node     = node->get_next();
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, pattern, fcfs_cs_data.value(), next_node);

  const EPLeaf leaf(ep_node, next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedSetAllocateAndWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibTessera