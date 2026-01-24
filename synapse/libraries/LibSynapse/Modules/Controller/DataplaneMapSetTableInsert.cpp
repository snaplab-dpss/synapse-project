#include <LibSynapse/Modules/Controller/DataplaneMapSetTableInsert.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::Table;

namespace {

struct map_set_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
};

map_set_table_data_t get_map_set_table_update_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const map_set_table_data_t data = {
      .obj = expr_addr_to_obj_addr(map_addr_expr),
      .key = key,
  };

  return data;
}

struct pattern_t {
  const Call *map_put;
  const Call *dchain_allocate_new_index;
  branch_direction_t index_alloc_success_direction;
  symbol_t new_index;
  symbol_t index_allocation_success;
};

bool is_write_pattern(const Context &ctx, const BDD *bdd, const BDDNode *node, pattern_t &pattern) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  pattern.index_alloc_success_direction = bdd->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (pattern.index_alloc_success_direction.branch == nullptr) {
    return false;
  }

  const BDDNode *on_index_allocation_success = pattern.index_alloc_success_direction.get_success_node();
  if (on_index_allocation_success->get_type() != BDDNodeType::Call) {
    return false;
  }

  const addr_t obj = expr_addr_to_obj_addr(dchain_allocate_new_index->get_call().args.at("chain").expr);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return false;
  }

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, map_objs.value(), future_map_puts) || future_map_puts.size() != 1) {
    return false;
  }

  pattern.map_put                   = future_map_puts[0];
  pattern.dchain_allocate_new_index = dchain_allocate_new_index;
  pattern.new_index                 = dchain_allocate_new_index->get_local_symbol("new_index");
  pattern.index_allocation_success  = dchain_allocate_new_index->get_local_symbol("not_out_of_space");

  return true;
}

std::unique_ptr<BDD> rebuild_bdd(EP *new_ep, const BDDNode *node, const pattern_t &pattern, BDDNode *&new_next) {
  const BDD *old_bdd       = new_ep->get_bdd();
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *next = node->get_next();

  BDDNode *replaced = bdd->delete_non_branch(pattern.map_put->get_id());

  Call *dchain_allocate_new_index = dynamic_cast<Call *>(bdd->get_mutable_node_by_id(pattern.dchain_allocate_new_index->get_id()));
  dchain_allocate_new_index->remove_local_symbol(pattern.new_index.name);

  if (pattern.map_put->get_id() == next->get_id()) {
    new_next = replaced;
  } else {
    new_next = bdd->get_mutable_node_by_id(node->get_next()->get_id());
  }

  return bdd;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapSetTableInsertFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  pattern_t pattern;
  if (!is_write_pattern(speculations.ctx, ep->get_bdd(), node, pattern)) {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_update_data(pattern.map_put);

  const std::optional<map_coalescing_objs_t> coalescing_objs = speculations.ctx.get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!speculations.ctx.check_ds_impl(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !speculations.ctx.check_ds_impl(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  spec_impl_t spec_impl(decide(ep, node), speculations.ctx);
  spec_impl.skip.insert(pattern.map_put->get_id());

  return spec_impl;
}

std::vector<impl_t> DataplaneMapSetTableInsertFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  pattern_t pattern;
  if (!is_write_pattern(ep->get_ctx(), ep->get_bdd(), node, pattern)) {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_update_data(pattern.map_put);

  const std::optional<map_coalescing_objs_t> coalescing_objs = ep->get_ctx().get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().check_ds_impl(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !ep->get_ctx().check_ds_impl(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  Module *module  = new DataplaneMapSetTableInsert(node, data.obj, data.key, pattern.index_allocation_success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  BDDNode *new_next;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, pattern, new_next);

  const EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMapSetTableInsertFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  pattern_t pattern;
  if (!is_write_pattern(ctx, bdd, node, pattern)) {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_update_data(pattern.map_put);

  const std::optional<map_coalescing_objs_t> coalescing_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !ctx.check_ds_impl(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapSetTableInsert>(node, data.obj, data.key, pattern.index_allocation_success);
}

} // namespace Controller
} // namespace LibSynapse