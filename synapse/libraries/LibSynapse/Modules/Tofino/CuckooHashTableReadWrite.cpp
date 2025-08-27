#include <LibSynapse/Modules/Tofino/CuckooHashTableReadWrite.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>

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

symbol_t create_cuckoo_success_symbol(DS_ID id, SymbolManager *symbol_manager, const BDDNode *node) {
  const std::string name = id + "_" + std::to_string(node->get_id());
  const bits_t size      = 8;
  return symbol_manager->create_symbol(name, size);
}

klee::ref<klee::Expr> build_cuckoo_success_condition(const symbol_t &cuckoo_success_symbol) {
  const bits_t width               = cuckoo_success_symbol.expr->getWidth();
  const klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  return solver_toolbox.exprBuilder->Ne(cuckoo_success_symbol.expr, zero);
}

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

struct read_write_pattern_t {
  klee::ref<klee::Expr> cuckoo_update_condition;
  const BDDNode *on_read_success;
  const BDDNode *on_read_failure;
  const BDDNode *on_write_success;
  const BDDNode *on_write_failure;
  const BDDNode *on_write_success2;
};

bool is_read_write_pattern_on_condition(const BDD *bdd, const BDDNode *node, const map_coalescing_objs_t &map_objs,
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
  if (!bdd->is_map_get_and_branch_checking_success(map_get, node->get_next(), map_get_success_direction)) {
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

  const branch_direction_t branch_checking_index_alloc = bdd->find_branch_checking_index_alloc(dynamic_cast<const Call *>(on_insert));
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

  on_insert = on_insert->get_next();

  if (!bdd->are_subtrees_equal(read_write_pattern.on_write_success, on_insert)) {
    return false;
  }

  read_write_pattern.on_write_success2 = on_insert;

  return true;
}

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const BDDNode *map_get, const read_write_pattern_t &read_write_pattern) {
  std::vector<const BDDNode *> nodes_to_ignore;

  map_get->get_next()->visit_nodes([&nodes_to_ignore, read_write_pattern](const BDDNode *node) {
    if (node == read_write_pattern.on_read_success || node == read_write_pattern.on_read_failure || node == read_write_pattern.on_write_success ||
        node == read_write_pattern.on_write_failure || node == read_write_pattern.on_write_success2) {
      return BDDNodeVisitAction::SkipChildren;
    }
    nodes_to_ignore.push_back(node);
    return BDDNodeVisitAction::Continue;
  });

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

  if (cuckoo_hash_table_data.key->getWidth() != CuckooHashTable::SUPPORTED_KEY_SIZE ||
      (!cuckoo_hash_table_data.read_value.isNull() && cuckoo_hash_table_data.read_value->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE) ||
      (!cuckoo_hash_table_data.write_value.isNull() && cuckoo_hash_table_data.write_value->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE)) {
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

  read_write_pattern_t read_write_pattern;
  if (!is_read_write_pattern_on_condition(ep->get_bdd(), node, map_objs.value(), read_write_pattern)) {
    return {};
  }

  // std::cerr << "cuckoo_update_condition: " << LibCore::expr_to_string(read_write_pattern.cuckoo_update_condition, true) << "\n";
  // std::cerr << "on_read_success: " << read_write_pattern.on_read_success->dump(true) << "\n";
  // std::cerr << "on_read_failure: " << read_write_pattern.on_read_failure->dump(true) << "\n";
  // std::cerr << "on_write_success: " << read_write_pattern.on_write_success->dump(true) << "\n";
  // std::cerr << "on_write_failure: " << read_write_pattern.on_write_failure->dump(true) << "\n";

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  std::vector<const BDDNode *> ignore_nodes = get_nodes_to_speculatively_ignore(ep, map_get, read_write_pattern);
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

  if (cuckoo_hash_table_data.key->getWidth() != CuckooHashTable::SUPPORTED_KEY_SIZE ||
      (!cuckoo_hash_table_data.read_value.isNull() && cuckoo_hash_table_data.read_value->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE) ||
      (!cuckoo_hash_table_data.write_value.isNull() && cuckoo_hash_table_data.write_value->getWidth() != CuckooHashTable::SUPPORTED_VALUE_SIZE)) {
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

  std::cerr << "Checkpoint!\n";
  dbg_pause();

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