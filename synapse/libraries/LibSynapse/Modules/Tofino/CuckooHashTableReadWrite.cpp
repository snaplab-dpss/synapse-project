#include <LibSynapse/Modules/Tofino/CuckooHashTableReadWrite.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>

#include <cmath>

namespace LibSynapse {
namespace Tofino {
namespace {

using LibBDD::Branch;
using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

struct cuckoo_hash_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  u32 capacity;

  cuckoo_hash_table_data_t(const Context &ctx, const Call *map_get) {
    const call_t &get_call = map_get->get_call();
    obj                    = expr_addr_to_obj_addr(get_call.args.at("map").expr);
    key                    = get_call.args.at("key").in;
    capacity               = ctx.get_map_config(obj).capacity;
  }
};

symbol_t create_cuckoo_success_symbol(DS_ID id, SymbolManager *symbol_manager, const BDDNode *node) {
  const std::string name = id + "_" + std::to_string(node->get_id()) + "_success";
  const bits_t size      = 8;
  return symbol_manager->create_symbol(name, size);
}

klee::ref<klee::Expr> build_cuckoo_success_condition(const symbol_t &cuckoo_success_symbol) {
  const bits_t width               = cuckoo_success_symbol.expr->getWidth();
  const klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  return solver_toolbox.exprBuilder->Ne(cuckoo_success_symbol.expr, zero);
}

struct read_write_pattern_t {
  klee::ref<klee::Expr> cuckoo_update_condition;
  const BDDNode *on_read_success;
  const BDDNode *on_read_failure;
  const BDDNode *on_write_success;
  const BDDNode *on_write_failure;
  const BDDNode *on_insert_success;
  symbol_t read_value;
  hit_rate_t on_read_failure_hr;
  hit_rate_t on_read_success_hr;
  hit_rate_t on_write_failure_hr;
  hit_rate_t on_write_success_hr;
  hit_rate_t on_insert_success_hr;
  hit_rate_t get_hr;
  hit_rate_t put_hr;
};

hit_rate_t calculate_expected_cache_hit_rate(const EP *ep, const BDDNode *node, const read_write_pattern_t &read_write_pattern,
                                             klee::ref<klee::Expr> key, u32 capacity) {
  const std::vector<klee::ref<klee::Expr>> constraints  = node->get_ordered_branch_constraints();
  const flow_stats_t flow_stats                         = ep->get_ctx().get_profiler().get_flow_stats(constraints, key);
  const hit_rate_t probability_of_finding_item_in_table = flow_stats.calculate_top_k_hit_rate(capacity);
  const hit_rate_t expected_cache_hit_rate              = 1_hr - ((1_hr - probability_of_finding_item_in_table) * 1.7);
  return expected_cache_hit_rate;
}

bool is_read_write_pattern_on_condition(const EP *ep, const BDDNode *node, const map_coalescing_objs_t &map_objs,
                                        read_write_pattern_t &read_write_pattern) {
  auto is_map_op = [map_objs](const BDDNode *map_node, const std::string &map_op_fname) {
    if (map_node->get_type() != BDDNodeType::Call) {
      return false;
    }
    const Call *call_node = dynamic_cast<const Call *>(map_node);
    const call_t &call    = call_node->get_call();
    if (call.function_name != map_op_fname) {
      return false;
    }
    return expr_addr_to_obj_addr(call.args.at("map").expr) == map_objs.map;
  };

  auto is_vector_op = [map_objs](const BDDNode *vector_node, const std::string &vector_op_fname) {
    if (vector_node->get_type() != BDDNodeType::Call) {
      return false;
    }
    const Call *call_node = dynamic_cast<const Call *>(vector_node);
    const call_t &call    = call_node->get_call();
    if (call.function_name != vector_op_fname) {
      return false;
    }
    return map_objs.vectors.find(expr_addr_to_obj_addr(call.args.at("vector").expr)) != map_objs.vectors.end();
  };

  auto is_dchain_op = [map_objs](const BDDNode *dchain_node, const std::string &dchain_op_fname) {
    if (dchain_node->get_type() != BDDNodeType::Call) {
      return false;
    }
    const Call *call_node = dynamic_cast<const Call *>(dchain_node);
    const call_t &call    = call_node->get_call();
    if (call.function_name != dchain_op_fname) {
      return false;
    }
    return expr_addr_to_obj_addr(call.args.at("chain").expr) == map_objs.dchain;
  };

  if (!is_map_op(node, "map_get")) {
    return false;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  bool map_get_success_direction;
  if (!ep->get_bdd()->is_map_get_and_branch_checking_success(map_get, node->get_next(), map_get_success_direction)) {
    return false;
  }

  const Branch *branch_checking_map_get_success = dynamic_cast<const Branch *>(node->get_next());

  const BDDNode *on_map_get_success =
      map_get_success_direction ? branch_checking_map_get_success->get_on_true() : branch_checking_map_get_success->get_on_false();
  const BDDNode *on_map_get_failure =
      map_get_success_direction ? branch_checking_map_get_success->get_on_false() : branch_checking_map_get_success->get_on_true();

  const Branch *on_map_get_success_branch = nullptr;
  const Branch *on_map_get_failure_branch = nullptr;

  while (on_map_get_success) {
    on_map_get_success = on_map_get_success->get_next();

    if (is_dchain_op(on_map_get_success, "dchain_rejuvenate_index")) {
      continue;
    }

    if (is_vector_op(on_map_get_success, "vector_borrow")) {
      read_write_pattern.read_value = dynamic_cast<const Call *>(on_map_get_success)->get_local_symbol("vector_data");
      continue;
    }

    if (on_map_get_success->get_type() == BDDNodeType::Branch) {
      on_map_get_success_branch = dynamic_cast<const Branch *>(on_map_get_success);
    }

    break;
  }

  if (on_map_get_failure->get_type() == BDDNodeType::Branch) {
    on_map_get_failure_branch = dynamic_cast<const Branch *>(on_map_get_failure);
  }

  if (on_map_get_success_branch == nullptr || on_map_get_failure_branch == nullptr) {
    return false;
  }

  if (!solver_toolbox.are_exprs_always_equal(on_map_get_success_branch->get_condition(), on_map_get_failure_branch->get_condition())) {
    return false;
  }

  read_write_pattern.cuckoo_update_condition = on_map_get_failure_branch->get_condition();

  const bool update_on_true_condition  = is_dchain_op(on_map_get_failure_branch->get_on_true(), "dchain_allocate_new_index");
  const bool update_on_false_condition = is_dchain_op(on_map_get_failure_branch->get_on_false(), "dchain_allocate_new_index");

  if (!update_on_true_condition && !update_on_false_condition) {
    return false;
  }

  read_write_pattern.on_read_failure =
      update_on_true_condition ? on_map_get_failure_branch->get_on_false() : on_map_get_failure_branch->get_on_true();

  const BDDNode *on_insert = update_on_true_condition ? on_map_get_failure_branch->get_on_true() : on_map_get_failure_branch->get_on_false();

  const BDDNode *vector_update = update_on_true_condition ? on_map_get_success_branch->get_on_true() : on_map_get_success_branch->get_on_false();
  const BDDNode *vector_read   = update_on_true_condition ? on_map_get_success_branch->get_on_false() : on_map_get_success_branch->get_on_true();

  if (!is_vector_op(vector_update, "vector_return") || !is_vector_op(vector_read, "vector_return")) {
    return false;
  }

  if (!dynamic_cast<const Call *>(vector_update)->is_vector_write() || !dynamic_cast<const Call *>(vector_read)->is_vector_read()) {
    return false;
  }

  if (!vector_update->get_next() || !vector_read->get_next()) {
    return false;
  }

  read_write_pattern.on_write_success = vector_update->get_next();
  read_write_pattern.on_read_success  = vector_read->get_next();

  const branch_direction_t branch_checking_index_alloc = ep->get_bdd()->find_branch_checking_index_alloc(dynamic_cast<const Call *>(on_insert));
  if (on_insert->get_next() != branch_checking_index_alloc.branch) {
    return false;
  }

  read_write_pattern.on_write_failure =
      branch_checking_index_alloc.direction ? branch_checking_index_alloc.branch->get_on_false() : branch_checking_index_alloc.branch->get_on_true();
  on_insert =
      branch_checking_index_alloc.direction ? branch_checking_index_alloc.branch->get_on_true() : branch_checking_index_alloc.branch->get_on_false();

  if (!is_map_op(on_insert, "map_put")) {
    return false;
  }

  on_insert = on_insert->get_next();

  if (!is_vector_op(on_insert, "vector_borrow")) {
    return false;
  }

  on_insert = on_insert->get_next();

  if (!is_vector_op(on_insert, "vector_return") || !dynamic_cast<const Call *>(on_insert)->is_vector_write()) {
    return false;
  }

  on_insert                            = on_insert->get_next();
  read_write_pattern.on_insert_success = on_insert;

  if (!ep->get_bdd()->are_subtrees_equal(read_write_pattern.on_write_success, read_write_pattern.on_insert_success)) {
    return false;
  }

  const Profiler &profiler = ep->get_ctx().get_profiler();

  read_write_pattern.on_read_success_hr   = profiler.get_hr(read_write_pattern.on_read_success);
  read_write_pattern.on_read_failure_hr   = profiler.get_hr(read_write_pattern.on_read_failure);
  read_write_pattern.on_write_success_hr  = profiler.get_hr(read_write_pattern.on_write_success);
  read_write_pattern.on_write_failure_hr  = profiler.get_hr(read_write_pattern.on_write_failure);
  read_write_pattern.on_insert_success_hr = profiler.get_hr(read_write_pattern.on_insert_success);

  read_write_pattern.get_hr = read_write_pattern.on_read_success_hr + read_write_pattern.on_read_failure_hr;
  read_write_pattern.put_hr =
      read_write_pattern.on_write_success_hr + read_write_pattern.on_write_failure_hr + read_write_pattern.on_insert_success_hr;

  return true;
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const BDDNode *old_map_get, const read_write_pattern_t &read_write_pattern,
                                 const symbol_t &cuckoo_success_symbol, klee::ref<klee::Expr> cuckoo_success_condition) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  Call *map_get = dynamic_cast<Call *>(new_bdd->get_mutable_node_by_id(old_map_get->get_id()));
  map_get->add_local_symbol(cuckoo_success_symbol);
  map_get->add_local_symbol(read_write_pattern.read_value);

  const bdd_node_ids_t stopping_nodes_ids{
      read_write_pattern.on_read_success->get_id(),
      read_write_pattern.on_read_failure->get_id(),
      read_write_pattern.on_write_success->get_id(),
      read_write_pattern.on_write_failure->get_id(),
  };

  const std::vector<LibBDD::BDDNode *> stopping_nodes = new_bdd->delete_until(old_map_get->get_next()->get_id(), stopping_nodes_ids);

  auto get_from_stopping_nodes = [&stopping_nodes](bdd_node_id_t id) {
    auto found_it = std::find_if(stopping_nodes.begin(), stopping_nodes.end(), [id](const LibBDD::BDDNode *n) { return n->get_id() == id; });
    assert(found_it != stopping_nodes.end());
    return *found_it;
  };

  BDDNode *on_read_success  = get_from_stopping_nodes(read_write_pattern.on_read_success->get_id());
  BDDNode *on_read_failure  = get_from_stopping_nodes(read_write_pattern.on_read_failure->get_id());
  BDDNode *on_write_success = get_from_stopping_nodes(read_write_pattern.on_write_success->get_id());
  BDDNode *on_write_failure = get_from_stopping_nodes(read_write_pattern.on_write_failure->get_id());

  Branch *cuckoo_update_condition_branch    = new_bdd->create_new_branch(read_write_pattern.cuckoo_update_condition);
  Branch *cuckoo_success_condition_branch_0 = new_bdd->create_new_branch(cuckoo_success_condition);
  Branch *cuckoo_success_condition_branch_1 = new_bdd->create_new_branch(cuckoo_success_condition);

  map_get->set_next(cuckoo_update_condition_branch);
  cuckoo_update_condition_branch->set_prev(map_get);

  cuckoo_update_condition_branch->set_on_true(cuckoo_success_condition_branch_0);
  cuckoo_update_condition_branch->set_on_false(cuckoo_success_condition_branch_1);

  cuckoo_success_condition_branch_0->set_prev(cuckoo_update_condition_branch);
  cuckoo_success_condition_branch_1->set_prev(cuckoo_update_condition_branch);

  cuckoo_success_condition_branch_0->set_on_true(on_write_success);
  cuckoo_success_condition_branch_0->set_on_false(on_write_failure);

  on_write_success->set_prev(cuckoo_success_condition_branch_0);
  on_write_failure->set_prev(cuckoo_success_condition_branch_0);

  cuckoo_success_condition_branch_1->set_on_true(on_read_success);
  cuckoo_success_condition_branch_1->set_on_false(on_read_failure);

  on_read_success->set_prev(cuckoo_success_condition_branch_1);
  on_read_failure->set_prev(cuckoo_success_condition_branch_1);

  return new_bdd;
}

void update_profiler(Profiler &profiler, const BDD *new_bdd, const BDDNode *old_map_get, const read_write_pattern_t &read_write_pattern,
                     klee::ref<klee::Expr> cuckoo_success_condition, hit_rate_t expected_cache_hit_rate) {
  const std::vector<klee::ref<klee::Expr>> map_get_constraints = old_map_get->get_ordered_branch_constraints();

  profiler.insert_relative(map_get_constraints, read_write_pattern.cuckoo_update_condition,
                           hit_rate_t(read_write_pattern.put_hr / (read_write_pattern.get_hr + read_write_pattern.put_hr)));

  std::vector<klee::ref<klee::Expr>> on_update_true_constraints = map_get_constraints;
  on_update_true_constraints.push_back(read_write_pattern.cuckoo_update_condition);

  std::vector<klee::ref<klee::Expr>> on_update_false_constraints = map_get_constraints;
  on_update_false_constraints.push_back(solver_toolbox.exprBuilder->Not(read_write_pattern.cuckoo_update_condition));

  profiler.insert_relative(on_update_true_constraints, cuckoo_success_condition, expected_cache_hit_rate);
  profiler.insert_relative(on_update_false_constraints, cuckoo_success_condition, expected_cache_hit_rate);

  std::vector<klee::ref<klee::Expr>> target_for_deletion;
  std::vector<klee::ref<klee::Expr>> stopping_point;

  target_for_deletion = on_update_true_constraints;
  target_for_deletion.push_back(cuckoo_success_condition);
  stopping_point = read_write_pattern.on_write_success->get_ordered_branch_constraints();
  stopping_point.push_back(read_write_pattern.cuckoo_update_condition);
  stopping_point.push_back(cuckoo_success_condition);
  profiler.remove_until(target_for_deletion, stopping_point);

  target_for_deletion = on_update_true_constraints;
  target_for_deletion.push_back(solver_toolbox.exprBuilder->Not(cuckoo_success_condition));
  stopping_point = read_write_pattern.on_write_failure->get_ordered_branch_constraints();
  stopping_point.push_back(read_write_pattern.cuckoo_update_condition);
  stopping_point.push_back(solver_toolbox.exprBuilder->Not(cuckoo_success_condition));
  profiler.remove_until(target_for_deletion, stopping_point);

  target_for_deletion = on_update_false_constraints;
  target_for_deletion.push_back(cuckoo_success_condition);
  stopping_point = read_write_pattern.on_read_success->get_ordered_branch_constraints();
  stopping_point.push_back(solver_toolbox.exprBuilder->Not(read_write_pattern.cuckoo_update_condition));
  stopping_point.push_back(cuckoo_success_condition);
  profiler.remove_until(target_for_deletion, stopping_point);

  target_for_deletion = on_update_false_constraints;
  target_for_deletion.push_back(solver_toolbox.exprBuilder->Not(cuckoo_success_condition));
  stopping_point = read_write_pattern.on_read_failure->get_ordered_branch_constraints();
  stopping_point.push_back(solver_toolbox.exprBuilder->Not(read_write_pattern.cuckoo_update_condition));
  stopping_point.push_back(solver_toolbox.exprBuilder->Not(cuckoo_success_condition));
  profiler.remove_until(target_for_deletion, stopping_point);

  // ProfilerViz::visualize(new_bdd, profiler, true);
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

  const cuckoo_hash_table_data_t cuckoo_hash_table_data(ep->get_ctx(), map_get);

  if (cuckoo_hash_table_data.key->getWidth() != CuckooHashTable::SUPPORTED_KEY_SIZE) {
    return {};
  }

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cuckoo_hash_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (map_objs->vectors.size() != 1) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(map_objs->dchain)) {
    return {};
  }

  if (ep->get_bdd()->is_dchain_used_for_index_allocation_queries(map_objs->dchain)) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_CuckooHashTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_CuckooHashTable) ||
      !ctx.can_impl_ds(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable)) {
    return {};
  }

  read_write_pattern_t read_write_pattern;
  if (!is_read_write_pattern_on_condition(ep, node, map_objs.value(), read_write_pattern)) {
    return {};
  }

  if (read_write_pattern.read_value.expr->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE) {
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

  const hit_rate_t expected_cache_hit_rate =
      calculate_expected_cache_hit_rate(ep, node, read_write_pattern, cuckoo_hash_table_data.key, cuckoo_hash_table_data.capacity);

  if (!ctx.get_profiler().can_set(read_write_pattern.on_read_failure->get_ordered_branch_constraints(),
                                  hit_rate_t(read_write_pattern.get_hr * (1 - expected_cache_hit_rate))) ||
      !ctx.get_profiler().can_set(read_write_pattern.on_write_failure->get_ordered_branch_constraints(),
                                  hit_rate_t(read_write_pattern.put_hr * (1 - expected_cache_hit_rate)))) {
    return {};
  }

  new_ctx.get_mutable_profiler().set(read_write_pattern.on_read_failure->get_ordered_branch_constraints(),
                                     hit_rate_t(read_write_pattern.get_hr * (1 - expected_cache_hit_rate)));
  new_ctx.get_mutable_profiler().set(read_write_pattern.on_write_failure->get_ordered_branch_constraints(),
                                     hit_rate_t(read_write_pattern.put_hr * (1 - expected_cache_hit_rate)));

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  map_get->get_next()->visit_nodes([&spec_impl, read_write_pattern](const BDDNode *future_node) {
    if (future_node == read_write_pattern.on_read_success || future_node == read_write_pattern.on_read_failure ||
        future_node == read_write_pattern.on_write_success || future_node == read_write_pattern.on_write_failure ||
        future_node == read_write_pattern.on_insert_success) {
      return BDDNodeVisitAction::SkipChildren;
    }
    spec_impl.skip.insert(future_node->get_id());
    return BDDNodeVisitAction::Continue;
  });

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

  const cuckoo_hash_table_data_t cuckoo_hash_table_data(ep->get_ctx(), map_get);

  if (cuckoo_hash_table_data.key->getWidth() != CuckooHashTable::SUPPORTED_KEY_SIZE) {
    return {};
  }

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(cuckoo_hash_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (map_objs->vectors.size() != 1) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(map_objs->dchain)) {
    return {};
  }

  if (ep->get_bdd()->is_dchain_used_for_index_allocation_queries(map_objs->dchain)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_CuckooHashTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_CuckooHashTable) ||
      !ep->get_ctx().can_impl_ds(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable)) {
    return {};
  }

  read_write_pattern_t read_write_pattern;
  if (!is_read_write_pattern_on_condition(ep, node, map_objs.value(), read_write_pattern)) {
    return {};
  }

  if (read_write_pattern.read_value.expr->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE) {
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

  const symbol_t cuckoo_success_symbol                 = create_cuckoo_success_symbol(cuckoo_hash_table->id, symbol_manager, node);
  const klee::ref<klee::Expr> cuckoo_success_condition = build_cuckoo_success_condition(cuckoo_success_symbol);

  Module *cuckoo_read_write_module =
      new CuckooHashTableReadWrite(node, cuckoo_hash_table->id, cuckoo_hash_table_data.obj, cuckoo_hash_table_data.key,
                                   read_write_pattern.read_value.expr, read_write_pattern.cuckoo_update_condition, cuckoo_success_condition);

  Module *if_cuckoo_update_module   = new If(node, read_write_pattern.cuckoo_update_condition);
  Module *then_cuckoo_update_module = new Then(node);
  Module *else_cuckoo_update_module = new Else(node);

  Module *if_cuckoo_success_on_cuckoo_update_module   = new If(node, cuckoo_success_condition);
  Module *then_cuckoo_success_on_cuckoo_update_module = new Then(node);
  Module *else_cuckoo_success_on_cuckoo_update_module = new Else(node);

  Module *if_cuckoo_success_on_cuckoo_read_module   = new If(node, cuckoo_success_condition);
  Module *then_cuckoo_success_on_cuckoo_read_module = new Then(node);
  Module *else_cuckoo_success_on_cuckoo_read_module = new Else(node);

  EPNode *cuckoo_read_write_ep_node = new EPNode(cuckoo_read_write_module);

  EPNode *if_cuckoo_update_ep_node   = new EPNode(if_cuckoo_update_module);
  EPNode *then_cuckoo_update_ep_node = new EPNode(then_cuckoo_update_module);
  EPNode *else_cuckoo_update_ep_node = new EPNode(else_cuckoo_update_module);

  EPNode *if_cuckoo_success_on_cuckoo_update_ep_node   = new EPNode(if_cuckoo_success_on_cuckoo_update_module);
  EPNode *then_cuckoo_success_on_cuckoo_update_ep_node = new EPNode(then_cuckoo_success_on_cuckoo_update_module);
  EPNode *else_cuckoo_success_on_cuckoo_update_ep_node = new EPNode(else_cuckoo_success_on_cuckoo_update_module);

  EPNode *if_cuckoo_success_on_cuckoo_read_ep_node   = new EPNode(if_cuckoo_success_on_cuckoo_read_module);
  EPNode *then_cuckoo_success_on_cuckoo_read_ep_node = new EPNode(then_cuckoo_success_on_cuckoo_read_module);
  EPNode *else_cuckoo_success_on_cuckoo_read_ep_node = new EPNode(else_cuckoo_success_on_cuckoo_read_module);

  cuckoo_read_write_ep_node->set_children(if_cuckoo_update_ep_node);
  if_cuckoo_update_ep_node->set_prev(cuckoo_read_write_ep_node);

  if_cuckoo_update_ep_node->set_children(read_write_pattern.cuckoo_update_condition, then_cuckoo_update_ep_node, else_cuckoo_update_ep_node);
  then_cuckoo_update_ep_node->set_prev(if_cuckoo_update_ep_node);
  else_cuckoo_update_ep_node->set_prev(if_cuckoo_update_ep_node);

  then_cuckoo_update_ep_node->set_children(if_cuckoo_success_on_cuckoo_update_ep_node);
  if_cuckoo_success_on_cuckoo_update_ep_node->set_prev(then_cuckoo_update_ep_node);

  else_cuckoo_update_ep_node->set_children(if_cuckoo_success_on_cuckoo_read_ep_node);
  if_cuckoo_success_on_cuckoo_read_ep_node->set_prev(else_cuckoo_update_ep_node);

  if_cuckoo_success_on_cuckoo_update_ep_node->set_children(cuckoo_success_condition, then_cuckoo_success_on_cuckoo_update_ep_node,
                                                           else_cuckoo_success_on_cuckoo_update_ep_node);
  then_cuckoo_success_on_cuckoo_update_ep_node->set_prev(if_cuckoo_success_on_cuckoo_update_ep_node);
  else_cuckoo_success_on_cuckoo_update_ep_node->set_prev(if_cuckoo_success_on_cuckoo_update_ep_node);

  if_cuckoo_success_on_cuckoo_read_ep_node->set_children(cuckoo_success_condition, then_cuckoo_success_on_cuckoo_read_ep_node,
                                                         else_cuckoo_success_on_cuckoo_read_ep_node);
  then_cuckoo_success_on_cuckoo_read_ep_node->set_prev(if_cuckoo_success_on_cuckoo_read_ep_node);
  else_cuckoo_success_on_cuckoo_read_ep_node->set_prev(if_cuckoo_success_on_cuckoo_read_ep_node);

  std::unique_ptr<BDD> new_bdd = rebuild_bdd(ep, node, read_write_pattern, cuckoo_success_symbol, cuckoo_success_condition);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const hit_rate_t expected_cache_hit_rate =
      calculate_expected_cache_hit_rate(ep, node, read_write_pattern, cuckoo_hash_table_data.key, cuckoo_hash_table_data.capacity);
  update_profiler(new_ep->get_mutable_ctx().get_mutable_profiler(), new_bdd.get(), node, read_write_pattern, cuckoo_success_condition,
                  expected_cache_hit_rate);

  new_ep->get_mutable_ctx().save_ds_impl(map_objs->map, DSImpl::Tofino_CuckooHashTable);
  new_ep->get_mutable_ctx().save_ds_impl(map_objs->dchain, DSImpl::Tofino_CuckooHashTable);
  new_ep->get_mutable_ctx().save_ds_impl(*map_objs->vectors.begin(), DSImpl::Tofino_CuckooHashTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, map_objs->map, cuckoo_hash_table);

  EPLeaf then_cuckoo_success_on_cuckoo_update_leaf(then_cuckoo_success_on_cuckoo_update_ep_node,
                                                   new_bdd->get_node_by_id(read_write_pattern.on_write_success->get_id()));
  EPLeaf else_cuckoo_success_on_cuckoo_update_leaf(else_cuckoo_success_on_cuckoo_update_ep_node,
                                                   new_bdd->get_node_by_id(read_write_pattern.on_write_failure->get_id()));
  EPLeaf then_cuckoo_success_on_cuckoo_read_leaf(then_cuckoo_success_on_cuckoo_read_ep_node,
                                                 new_bdd->get_node_by_id(read_write_pattern.on_read_success->get_id()));
  EPLeaf else_cuckoo_success_on_cuckoo_read_leaf(else_cuckoo_success_on_cuckoo_read_ep_node,
                                                 new_bdd->get_node_by_id(read_write_pattern.on_read_failure->get_id()));
  new_ep->process_leaf(cuckoo_read_write_ep_node, {then_cuckoo_success_on_cuckoo_update_leaf, else_cuckoo_success_on_cuckoo_update_leaf,
                                                   then_cuckoo_success_on_cuckoo_read_leaf, else_cuckoo_success_on_cuckoo_read_leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CuckooHashTableReadWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const { return {}; }

} // namespace Tofino
} // namespace LibSynapse