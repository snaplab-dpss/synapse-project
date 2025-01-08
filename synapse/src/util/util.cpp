#include <algorithm>

#include "util.h"
#include "simplifier.h"
#include "solver.h"
#include "symbol.h"
#include "../constants.h"
#include "../bdd/bdd.h"
#include "../targets/targets.h"
#include "../execution_plan/execution_plan.h"

namespace synapse {
namespace {
struct obj_op_t {
  addr_t obj;
  const Call *call_node;

  bool operator==(const obj_op_t &other) const { return obj == other.obj; }
};

struct obj_op_hash_t {
  size_t operator()(const obj_op_t &obj_op) const { return obj_op.obj; }
};

typedef std::unordered_set<obj_op_t, obj_op_hash_t> objs_ops_t;

struct next_t {
  objs_ops_t maps;
  objs_ops_t vectors;
  objs_ops_t dchains;

  int size() const { return maps.size() + vectors.size(); }

  void intersect(const next_t &other) {
    auto intersector = [](objs_ops_t &a, const objs_ops_t &b) {
      for (auto it = a.begin(); it != a.end();) {
        if (b.find(*it) == b.end()) {
          it = a.erase(it);
        } else {
          it++;
        }
      }
    };

    intersector(maps, other.maps);
    intersector(vectors, other.vectors);
    intersector(dchains, other.dchains);
  }

  bool has_obj(addr_t obj) const {
    obj_op_t mock{obj, nullptr};
    if (maps.find(mock) != maps.end()) {
      return true;
    }

    if (vectors.find(mock) != vectors.end()) {
      return true;
    }

    if (dchains.find(mock) != dchains.end()) {
      return true;
    }

    return false;
  }
};

next_t get_next_maps_and_vectors(const Node *root, klee::ref<klee::Expr> index) {
  next_t candidates;

  root->visit_nodes([&candidates, index](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name == "map_put") {
      klee::ref<klee::Expr> map = call.args.at("map").expr;
      klee::ref<klee::Expr> value = call.args.at("value").expr;

      addr_t map_addr = expr_addr_to_obj_addr(map);
      bool same_index = solver_toolbox.are_exprs_always_equal(index, value);

      if (same_index) {
        candidates.maps.insert({map_addr, call_node});
      }
    }

    else if (call.function_name == "vector_borrow") {
      klee::ref<klee::Expr> vector = call.args.at("vector").expr;
      klee::ref<klee::Expr> value = call.args.at("index").expr;

      addr_t vector_addr = expr_addr_to_obj_addr(vector);
      bool same_index = solver_toolbox.are_exprs_always_equal(index, value);

      if (same_index) {
        candidates.vectors.insert({vector_addr, call_node});
      }
    }

    return NodeVisitAction::Continue;
  });

  return candidates;
}

next_t get_allowed_coalescing_objs(std::vector<const Call *> index_allocators, addr_t obj) {
  next_t candidates;

  for (const Call *allocator : index_allocators) {
    const call_t &allocator_call = allocator->get_call();

    klee::ref<klee::Expr> allocated_index = allocator_call.args.at("index_out").out;
    klee::ref<klee::Expr> dchain = allocator_call.args.at("chain").expr;
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain);

    // We expect the coalescing candidates to be the same regardless of
    // where in the BDD we are. In case there is some discrepancy, then it
    // should be invalid. We thus consider only the intersection of all
    // candidates.

    next_t new_candidates = get_next_maps_and_vectors(allocator, allocated_index);
    new_candidates.dchains.insert({dchain_addr, allocator});

    if (!new_candidates.has_obj(obj)) {
      continue;
    }

    if (candidates.size() == 0) {
      candidates = new_candidates;
    } else {

      // If we can't find the current candidates in the new candidates' list,
      // then it is not true that we can coalesce them in every scenario of
      // the NF.

      candidates.intersect(new_candidates);
    }
  }

  return candidates;
}

std::vector<const Call *> get_unfiltered_coalescing_nodes(const Node *node,
                                                          const map_coalescing_objs_t &data) {
  const std::vector<std::string> target_functions{
      "map_get",
      "map_put",
      "map_erase",
      "vector_borrow",
      "vector_return",
      "dchain_allocate_new_index",
      "dchain_rejuvenate_index",
      "dchain_expire_index",
      "dchain_is_index_allocated",
      "dchain_free_index",
  };

  std::vector<const Call *> unfiltered_nodes = node->get_future_functions(target_functions);

  auto filter_map_objs = [&data](const Node *node) {
    assert(node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.args.find("map") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);
      if (obj != data.map) {
        return true;
      }
    } else if (call.args.find("vector") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);
      if (data.vectors.find(obj) == data.vectors.end()) {
        return true;
      }
    } else if (call.args.find("chain") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);
      if (obj != data.dchain) {
        return true;
      }
    } else {
      return true;
    }

    return false;
  };

  unfiltered_nodes.erase(
      std::remove_if(unfiltered_nodes.begin(), unfiltered_nodes.end(), filter_map_objs),
      unfiltered_nodes.end());

  return unfiltered_nodes;
}

std::pair<hit_rate_t, std::string> n2hr(u64 n) {
  if (n < 1e3) {
    return {(hit_rate_t)n, ""};
  }

  if (n < 1e6) {
    return {n / 1e3, "K"};
  }

  if (n < 1e9) {
    return {n / 1e6, "M"};
  }

  if (n < 1e12) {
    return {n / 1e9, "G"};
  }

  return {n / 1e12, "T"};
}

addr_t get_vector_obj_storing_map_key(const BDD *bdd,
                                      const map_coalescing_objs_t &map_coalescing_objs) {
  std::vector<const Call *> vector_borrows =
      bdd->get_root()->get_future_functions({"vector_borrow"});

  for (const Call *vector_borrow : vector_borrows) {
    const call_t &vb = vector_borrow->get_call();

    klee::ref<klee::Expr> vector_expr = vb.args.at("vector").expr;
    klee::ref<klee::Expr> value_addr_expr = vb.args.at("val_out").out;

    addr_t vector_obj = expr_addr_to_obj_addr(vector_expr);
    addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    if (map_coalescing_objs.vectors.find(vector_obj) == map_coalescing_objs.vectors.end()) {
      continue;
    }

    std::vector<const Call *> map_puts = vector_borrow->get_future_functions({"map_put"});

    bool is_vector_key = false;
    for (const Call *map_put : map_puts) {
      const call_t &mp = map_put->get_call();

      klee::ref<klee::Expr> map_expr = mp.args.at("map").expr;
      klee::ref<klee::Expr> key_addr_expr = mp.args.at("key").expr;

      addr_t map_obj = expr_addr_to_obj_addr(map_expr);
      addr_t key_addr = expr_addr_to_obj_addr(key_addr_expr);

      if (map_obj != map_coalescing_objs.map) {
        continue;
      }

      if (key_addr == value_addr) {
        is_vector_key = true;
        break;
      }
    }

    if (!is_vector_key) {
      continue;
    }

    return vector_obj;
  }

  panic("Vector key not found");
}

void delete_all_vector_key_operations_from_bdd(BDD *bdd, addr_t map) {
  map_coalescing_objs_t map_coalescing_data;
  if (!get_map_coalescing_objs_from_bdd(bdd, map, map_coalescing_data)) {
    return;
  }

  addr_t vector_key = get_vector_obj_storing_map_key(bdd, map_coalescing_data);
  std::unordered_set<const Node *> candidates;

  bdd->get_root()->visit_nodes([map_coalescing_data, vector_key, &candidates](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return NodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    if (obj != vector_key) {
      return NodeVisitAction::Continue;
    }

    candidates.insert(node);

    return NodeVisitAction::Continue;
  });

  for (const Node *node : candidates) {
    bdd->delete_non_branch(node->get_id());
  }
}

} // namespace

pps_t bps2pps(bps_t bps, bytes_t pkt_size) {
  return bps / ((PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8);
}

bps_t pps2bps(pps_t pps, bytes_t pkt_size) {
  return pps * (PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8;
}

// Only for pairs of std::hash-able types for simplicity.
// You can of course template this struct to allow other hash functions
struct pair_hash {
  template <class T1, class T2> std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);

    // Mainly for demonstration purposes, i.e. works but is overly simple
    // In the real world, use sth. like boost.hash_combine
    return h1 ^ h2;
  }
};

std::vector<mod_t> build_hdr_modifications(const Call *packet_borrow_next_chunk,
                                           const Call *packet_return_chunk) {
  static std::unordered_map<node_id_t, std::vector<mod_t>> cache;

  auto cache_found_it = cache.find(packet_return_chunk->get_id());
  if (cache_found_it != cache.end()) {
    return cache_found_it->second;
  }

  const call_t &ret_call = packet_return_chunk->get_call();
  assert(ret_call.function_name == "packet_return_chunk" && "Unexpected function");

  const call_t &bor_call = packet_borrow_next_chunk->get_call();
  assert(bor_call.function_name == "packet_borrow_next_chunk" && "Unexpected function");

  klee::ref<klee::Expr> borrowed = bor_call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned = ret_call.args.at("the_chunk").in;
  assert(borrowed->getWidth() == returned->getWidth() && "Different widths");

  std::vector<mod_t> changes = build_expr_mods(borrowed, returned);

  cache[packet_return_chunk->get_id()] = changes;

  return changes;
}

// This is somewhat of a hack... We assume that checksum expressions will only
// be used to modify checksum fields of a packet, not other packet fields.
std::vector<mod_t> ignore_checksum_modifications(const std::vector<mod_t> &modifications) {
  std::vector<mod_t> filtered;

  for (const mod_t &mod : modifications) {
    klee::ref<klee::Expr> simplified = simplify(mod.expr);
    std::unordered_set<std::string> symbols = symbol_t::get_symbols_names(simplified);

    if (symbols.size() == 1 && simplified->getWidth() == 8) {
      const std::string &symbol = *symbols.begin();

      if (symbol.find("checksum") != std::string::npos) {
        continue;
      }
    }

    filtered.emplace_back(mod.offset, mod.width, simplified);
  }

  return filtered;
}

const Call *packet_borrow_from_return(const EP *ep, const Call *packet_return_chunk) {
  const call_t &call = packet_return_chunk->get_call();
  assert(call.function_name == "packet_return_chunk" && "Unexpected function");

  klee::ref<klee::Expr> chunk_returned = call.args.at("the_chunk").in;

  std::vector<const Call *> prev_borrows = packet_return_chunk->get_prev_functions(
      {"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const Call *> prev_returns = packet_return_chunk->get_prev_functions(
      {"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  assert(prev_borrows.size() && "No previous borrows");
  assert(prev_borrows.size() > prev_returns.size() && "No previous borrow");

  return prev_borrows[prev_borrows.size() - 1 - prev_returns.size()];
}

bool get_map_coalescing_objs_from_bdd(const BDD *bdd, addr_t obj, map_coalescing_objs_t &data) {
  const Node *root = bdd->get_root();

  std::vector<const Call *> index_allocators =
      root->get_future_functions({"dchain_allocate_new_index"});

  if (index_allocators.empty()) {
    return false;
  }

  next_t candidates = get_allowed_coalescing_objs(index_allocators, obj);

  if (candidates.size() == 0) {
    return false;
  }

  assert(candidates.maps.size() == 1 && "Expecting a single map");
  assert(candidates.dchains.size() == 1 && "Expecting a single dchain");

  data.map = candidates.maps.begin()->obj;
  data.dchain = candidates.dchains.begin()->obj;

  for (const auto &vector : candidates.vectors) {
    data.vectors.insert(vector.obj);
  }

  return true;
}

rw_fractions_t get_cond_map_put_rw_profile_fractions(const EP *ep, const Node *node) {
  const Context &ctx = ep->get_ctx();
  const Profiler &profiler = ctx.get_profiler();

  assert(node->get_type() == NodeType::Call && "Unexpected node type");
  const Call *map_get = dynamic_cast<const Call *>(node);

  const call_t &mg_call = map_get->get_call();
  assert(mg_call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  symbol_t map_has_this_key = map_get->get_local_symbol("map_has_this_key");

  branch_direction_t success_check = map_get->get_map_get_success_check();
  assert(success_check.branch && "Map get success check not found");

  const Node *read = success_check.direction ? success_check.branch->get_on_true()
                                             : success_check.branch->get_on_false();
  const Node *write_attempt = success_check.direction ? success_check.branch->get_on_false()
                                                      : success_check.branch->get_on_true();

  std::vector<const Call *> future_map_puts = write_attempt->get_future_functions({"map_put"});
  assert(future_map_puts.size() >= 1 && "map_put not found");

  const Node *write = nullptr;
  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> o = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> k = mp_call.args.at("key").in;

    if (solver_toolbox.are_exprs_always_equal(o, obj) &&
        solver_toolbox.are_exprs_always_equal(k, key)) {
      write = map_put;
      break;
    }
  }

  assert(write && "map_put not found");

  rw_fractions_t fractions;
  fractions.read = profiler.get_hr(read);
  fractions.write_attempt = profiler.get_hr(write_attempt);
  fractions.write = profiler.get_hr(write);

  return fractions;
}

bool are_nodes_equivalent(const Node *node0, const Node *node1) {
  if (node0->get_type() != node1->get_type()) {
    return false;
  }

  switch (node0->get_type()) {
  case NodeType::Call: {
    const Call *call0 = dynamic_cast<const Call *>(node0);
    const Call *call1 = dynamic_cast<const Call *>(node1);

    const call_t &call0_call = call0->get_call();
    const call_t &call1_call = call1->get_call();

    if (call0_call.function_name != call1_call.function_name) {
      return false;
    }

    if (call0_call.args.size() != call1_call.args.size()) {
      return false;
    }

    for (const auto &[arg_name, call0_arg] : call0_call.args) {
      if (call1_call.args.find(arg_name) == call1_call.args.end()) {
        return false;
      }

      const auto &call1_arg = call1_call.args.at(arg_name);

      if (!solver_toolbox.are_exprs_always_equal(call0_arg.expr, call1_arg.expr)) {
        return false;
      }

      if (!solver_toolbox.are_exprs_always_equal(call0_arg.in, call1_arg.in)) {
        return false;
      }

      if (!solver_toolbox.are_exprs_always_equal(call0_arg.out, call1_arg.out)) {
        return false;
      }

      if (call0_arg.fn_ptr_name.first != call1_arg.fn_ptr_name.first) {
        return false;
      }

      if (call0_arg.fn_ptr_name.first &&
          call0_arg.fn_ptr_name.second != call1_arg.fn_ptr_name.second) {
        return false;
      }
    }

    for (const auto &[name, call0_extra_var] : call0_call.extra_vars) {
      if (call1_call.extra_vars.find(name) == call1_call.extra_vars.end()) {
        return false;
      }

      const auto &call1_extra_var = call1_call.extra_vars.at(name);

      if (!solver_toolbox.are_exprs_always_equal(call0_extra_var.first, call1_extra_var.first)) {
        return false;
      }

      if (!solver_toolbox.are_exprs_always_equal(call0_extra_var.second, call1_extra_var.second)) {
        return false;
      }
    }

    if (!solver_toolbox.are_exprs_always_equal(call0_call.ret, call1_call.ret)) {
      return false;
    }
  } break;
  case NodeType::Branch: {
    const Branch *branch0 = dynamic_cast<const Branch *>(node0);
    const Branch *branch1 = dynamic_cast<const Branch *>(node1);

    if (!solver_toolbox.are_exprs_always_equal(branch0->get_condition(),
                                               branch1->get_condition())) {
      return false;
    }
  } break;
  case NodeType::Route: {
    const Route *route0 = dynamic_cast<const Route *>(node0);
    const Route *route1 = dynamic_cast<const Route *>(node1);

    if (route0->get_operation() != route1->get_operation()) {
      return false;
    }

    if (route0->get_dst_device() != route1->get_dst_device()) {
      return false;
    }
  } break;
  }

  return true;
}

bool are_node_paths_equivalent(const Node *node0, const Node *node1) {
  static std::map<std::pair<const Node *, const Node *>, bool> cache;

  auto found_cache_it = cache.find({node0, node1});
  if (found_cache_it != cache.end()) {
    return found_cache_it->second;
  }

  std::vector<const Node *> nodes0{node0};
  std::vector<const Node *> nodes1{node1};

  while (!nodes0.empty()) {
    const Node *current0 = nodes0.back();
    nodes0.pop_back();

    const Node *current1 = nodes1.back();
    nodes1.pop_back();

    if (!are_nodes_equivalent(current0, current1)) {
      cache[{node0, node1}] = false;
      return false;
    }

    std::vector<const Node *> current0_children = current0->get_children();
    std::vector<const Node *> current1_children = current1->get_children();

    if (current0_children.size() != current1_children.size()) {
      cache[{node0, node1}] = false;
      return false;
    }

    nodes0.insert(nodes0.end(), current0_children.begin(), current0_children.end());
    nodes1.insert(nodes1.end(), current1_children.begin(), current1_children.end());
  }

  cache[{node0, node1}] = true;

  return true;
}

bool is_compact_map_get_followed_by_map_put_on_miss(const EP *ep, const Call *map_get,
                                                    map_rw_pattern_t &map_rw_pattern) {
  static std::unordered_map<const Call *, std::optional<map_rw_pattern_t>> cache;

  if (cache.find(map_get) != cache.end()) {
    if (!cache[map_get].has_value()) {
      return false;
    }
    map_rw_pattern = cache[map_get].value();
    return true;
  }

  // 1. Check if the call is a map_get.

  const call_t &mg_call = map_get->get_call();

  if (mg_call.function_name != "map_get") {
    cache[map_get] = std::nullopt;
    return false;
  }

  map_rw_pattern.map_get = map_get;
  map_rw_pattern.map_get_success_check = map_get->get_map_get_success_check();

  if (!map_rw_pattern.map_get_success_check.branch ||
      map_get->get_next() != map_rw_pattern.map_get_success_check.branch) {
    cache[map_get] = std::nullopt;
    return false;
  }

  // 2. Checking for an extra condition before performing the map_put.

  const Node *on_failed_map_get = map_rw_pattern.map_get_success_check.direction
                                      ? map_rw_pattern.map_get_success_check.branch->get_on_false()
                                      : map_rw_pattern.map_get_success_check.branch->get_on_true();

  if (on_failed_map_get->get_type() == NodeType::Branch) {
    const Branch *write_extra_condition = dynamic_cast<const Branch *>(on_failed_map_get);

    const Node *on_true = write_extra_condition->get_on_true();
    const Node *on_false = write_extra_condition->get_on_false();

    std::vector<const Call *> on_true_mp = on_true->get_future_functions({"map_put"});
    std::vector<const Call *> on_false_mp = on_false->get_future_functions({"map_put"});

    if (on_true_mp.size() == 1 && on_false_mp.size() == 0) {
      map_rw_pattern.map_put_extra_condition.branch = write_extra_condition;
      map_rw_pattern.map_put_extra_condition.direction = true;
      on_failed_map_get = on_true;
    } else if (on_true_mp.size() == 0 && on_false_mp.size() == 1) {
      map_rw_pattern.map_put_extra_condition.branch = write_extra_condition;
      map_rw_pattern.map_put_extra_condition.direction = false;
      on_failed_map_get = on_false;
    } else {
      cache[map_get] = std::nullopt;
      return false;
    }
  }

  // 3. Checking the dchain index allocation operation on map_get miss.

  if (on_failed_map_get->get_type() != NodeType::Call) {
    cache[map_get] = std::nullopt;
    return false;
  }

  map_rw_pattern.dchain_allocate_new_index = dynamic_cast<const Call *>(on_failed_map_get);

  const call_t &dchain_allocate_new_index_call =
      map_rw_pattern.dchain_allocate_new_index->get_call();

  if (dchain_allocate_new_index_call.function_name != "dchain_allocate_new_index") {
    cache[map_get] = std::nullopt;
    return false;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    cache[map_get] = std::nullopt;
    return false;
  }

  if (expr_addr_to_obj_addr(dchain_allocate_new_index_call.args.at("chain").expr) !=
      map_objs.dchain) {
    cache[map_get] = std::nullopt;
    return false;
  }

  map_rw_pattern.index_alloc_check =
      map_rw_pattern.dchain_allocate_new_index->find_branch_checking_index_alloc();

  if (!map_rw_pattern.index_alloc_check.branch ||
      map_rw_pattern.dchain_allocate_new_index->get_next() !=
          map_rw_pattern.index_alloc_check.branch) {
    cache[map_get] = std::nullopt;
    return false;
  }

  // 4. If there are extra conditions, check that the node path on failed extra
  // conditions and failed index allocation is the same.
  if (map_rw_pattern.map_put_extra_condition.branch) {
    const Node *on_failed_extra_condition =
        map_rw_pattern.map_put_extra_condition.direction
            ? map_rw_pattern.map_put_extra_condition.branch->get_on_false()
            : map_rw_pattern.map_put_extra_condition.branch->get_on_true();

    const Node *on_failed_index_alloc =
        map_rw_pattern.index_alloc_check.direction
            ? map_rw_pattern.index_alloc_check.branch->get_on_false()
            : map_rw_pattern.index_alloc_check.branch->get_on_true();

    if (!are_node_paths_equivalent(on_failed_extra_condition, on_failed_index_alloc)) {
      cache[map_get] = std::nullopt;
      return false;
    }
  }

  // 5. Check for map_put operations with the allocated index.

  const Node *on_index_alloc = map_rw_pattern.index_alloc_check.direction
                                   ? map_rw_pattern.index_alloc_check.branch->get_on_true()
                                   : map_rw_pattern.index_alloc_check.branch->get_on_false();
  const Node *on_failed_index_alloc = map_rw_pattern.index_alloc_check.direction
                                          ? map_rw_pattern.index_alloc_check.branch->get_on_false()
                                          : map_rw_pattern.index_alloc_check.branch->get_on_true();

  map_rw_pattern.map_put = nullptr;

  for (const Call *map_put : on_index_alloc->get_future_functions({"map_put"})) {
    if (!Call::are_map_read_write_counterparts(map_get, map_put)) {
      continue;
    }

    if (map_rw_pattern.map_put) {
      // Multiple map_put operations found.
      cache[map_get] = std::nullopt;
      return false;
    }

    map_rw_pattern.map_put = map_put;
  }

  for (const Call *map_put : on_failed_index_alloc->get_future_functions({"map_put"})) {
    if (Call::are_map_read_write_counterparts(map_get, map_put)) {
      // map_put operations found on failed index allocation.
      cache[map_get] = std::nullopt;
      return false;
    }
  }

  cache[map_get] = map_rw_pattern;
  return true;
}

bool is_tb_tracing_check_followed_by_update_on_true(const Call *tb_is_tracing,
                                                    const Call *&tb_update_and_check) {
  const call_t &is_tracing_call = tb_is_tracing->get_call();

  if (is_tracing_call.function_name != "tb_is_tracing") {
    return false;
  }

  klee::ref<klee::Expr> tb = is_tracing_call.args.at("tb").expr;
  klee::ref<klee::Expr> is_tracing_condition = solver_toolbox.exprBuilder->Ne(
      is_tracing_call.ret,
      solver_toolbox.exprBuilder->Constant(0, is_tracing_call.ret->getWidth()));

  std::vector<const Call *> tb_update_and_checks =
      tb_is_tracing->get_future_functions({"tb_update_and_check"});

  tb_update_and_check = nullptr;
  for (const Call *candidate : tb_update_and_checks) {
    klee::ref<klee::Expr> candidate_tb = candidate->get_call().args.at("tb").expr;
    if (!solver_toolbox.are_exprs_always_equal(tb, candidate_tb)) {
      continue;
    }

    klee::ConstraintManager constraints = candidate->get_constraints();
    if (!solver_toolbox.is_expr_always_true(constraints, is_tracing_condition)) {
      continue;
    }

    tb_update_and_check = candidate;
    break;
  }

  return tb_update_and_check != nullptr;
}

bool is_map_update_with_dchain(const EP *ep, const Call *dchain_allocate_new_index,
                               std::vector<const Call *> &map_puts) {
  const call_t &call = dchain_allocate_new_index->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return false;
  }

  branch_direction_t index_alloc_check =
      dchain_allocate_new_index->find_branch_checking_index_alloc();

  if (!index_alloc_check.branch) {
    return false;
  }

  klee::ref<klee::Expr> condition = index_alloc_check.branch->get_condition();

  std::vector<const Call *> future_map_puts =
      dchain_allocate_new_index->get_future_functions({"map_put"});

  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> map_expr = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> mp_key = mp_call.args.at("key").in;
    klee::ref<klee::Expr> mp_value = mp_call.args.at("value").expr;

    addr_t map = expr_addr_to_obj_addr(map_expr);

    if (map != map_objs.map) {
      continue;
    }

    if (key.isNull()) {
      key = mp_key;
    } else if (!solver_toolbox.are_exprs_always_equal(key, mp_key)) {
      return false;
    }

    if (value.isNull()) {
      value = mp_value;
    } else if (!solver_toolbox.are_exprs_always_equal(value, mp_value)) {
      return false;
    }

    klee::ConstraintManager constraints = map_put->get_constraints();
    if (!solver_toolbox.is_expr_always_true(constraints, condition)) {
      return false;
    }

    map_puts.push_back(map_put);
  }

  if (map_puts.empty()) {
    return false;
  }

  return true;
}

bool is_index_alloc_on_unsuccessful_map_get(const EP *ep, const Call *dchain_allocate_new_index) {
  const call_t &call = dchain_allocate_new_index->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return false;
  }

  const Node *map_get = dchain_allocate_new_index;
  while (map_get) {
    if (map_get->get_type() != NodeType::Call) {
      map_get = map_get->get_prev();
      continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(map_get);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_get") {
      map_get = map_get->get_prev();
      continue;
    }

    klee::ref<klee::Expr> obj = call.args.at("map").expr;
    if (expr_addr_to_obj_addr(obj) != map_objs.map) {
      map_get = map_get->get_prev();
      continue;
    }

    break;
  }

  if (!map_get) {
    return false;
  }

  symbol_t map_has_this_key =
      dynamic_cast<const Call *>(map_get)->get_local_symbol("map_has_this_key");

  klee::ConstraintManager constraints = dchain_allocate_new_index->get_constraints();

  klee::ref<klee::Expr> found_key = solver_toolbox.exprBuilder->Ne(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  return solver_toolbox.is_expr_always_false(constraints, found_key);
}

bool is_map_get_followed_by_map_erases_on_hit(const Call *map_get,
                                              std::vector<const Call *> &map_erases) {
  const call_t &mg_call = map_get->get_call();

  if (mg_call.function_name != "map_get") {
    return false;
  }

  klee::ref<klee::Expr> obj_expr = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);
  symbol_t map_has_this_key = map_get->get_local_symbol("map_has_this_key");

  klee::ref<klee::Expr> successful_map_get = solver_toolbox.exprBuilder->Ne(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  std::vector<const Call *> future_map_erases = map_get->get_future_functions({"map_erase"});

  for (const Call *map_erase : future_map_erases) {
    const call_t &me_call = map_erase->get_call();
    assert(me_call.function_name == "map_erase" && "Unexpected function");

    klee::ref<klee::Expr> map_expr = me_call.args.at("map").expr;
    klee::ref<klee::Expr> me_key = me_call.args.at("key").in;

    addr_t map = expr_addr_to_obj_addr(map_expr);

    if (map != obj) {
      continue;
    }

    if (!solver_toolbox.are_exprs_always_equal(key, me_key)) {
      return false;
    }

    klee::ConstraintManager constraints = map_erase->get_constraints();
    if (!solver_toolbox.is_expr_always_true(constraints, successful_map_get)) {
      // Found map_put that happens even if map_get was successful.
      return false;
    }

    map_erases.push_back(map_erase);
  }

  if (map_erases.empty()) {
    return false;
  }

  return true;
}

std::vector<const Call *> get_coalescing_nodes_from_key(const Node *node,
                                                        klee::ref<klee::Expr> target_key,
                                                        const map_coalescing_objs_t &data) {
  std::vector<const Call *> filtered_nodes = get_unfiltered_coalescing_nodes(node, data);

  if (filtered_nodes.empty()) {
    return filtered_nodes;
  }

  klee::ref<klee::Expr> index;

  auto filter_map_nodes_and_retrieve_index = [&target_key, &index](const Node *node) {
    assert(node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.args.find("key") == call.args.end()) {
      return false;
    }

    klee::ref<klee::Expr> key = call.args.at("key").in;
    bool same_key = solver_toolbox.are_exprs_always_equal(key, target_key);

    if (same_key) {
      if (call.function_name == "map_get") {
        index = call.args.at("value_out").out;
      } else if (call.function_name == "map_put") {
        index = call.args.at("value").expr;
      }
    }

    return !same_key;
  };

  filtered_nodes.erase(std::remove_if(filtered_nodes.begin(), filtered_nodes.end(),
                                      filter_map_nodes_and_retrieve_index),
                       filtered_nodes.end());

  auto filter_vectors_nodes = [&index](const Node *node) {
    assert(node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return false;
    }

    assert(!index.isNull() && "Index is null");

    klee::ref<klee::Expr> value = call.args.at("index").expr;
    return !solver_toolbox.are_exprs_always_equal(index, value);
  };

  filtered_nodes.erase(
      std::remove_if(filtered_nodes.begin(), filtered_nodes.end(), filter_vectors_nodes),
      filtered_nodes.end());

  return filtered_nodes;
}

std::string int2hr(u64 value) {
  std::stringstream ss;
  std::string str = std::to_string(value);

  // Add thousands separator
  for (int i = str.size() - 3; i > 0; i -= 3) {
    str.insert(i, 1, THOUSANDS_SEPARATOR);
  }

  ss << str;
  return ss.str();
}

std::string tput2str(u64 thpt, const std::string &units, bool human_readable) {
  std::stringstream ss;

  if (human_readable) {
    auto [n, m] = n2hr(thpt);

    ss.setf(std::ios::fixed);
    ss.precision(2);

    ss << n;
    ss << " ";
    ss << m;
  } else {
    std::string str = std::to_string(thpt);

    // Add thousands separator
    for (int i = str.size() - 3; i > 0; i -= 3) {
      str.insert(i, 1, THOUSANDS_SEPARATOR);
    }

    ss << str;
    ss << " ";
  }

  ss << units;
  return ss.str();
}

bool get_map_coalescing_objs_from_dchain_op(const EP *ep, const Call *dchain_op,
                                            map_coalescing_objs_t &map_objs) {
  const call_t &call = dchain_op->get_call();

  assert(call.args.find("chain") != call.args.end() && "No chain argument");
  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const Context &ctx = ep->get_ctx();
  std::optional<map_coalescing_objs_t> data = ctx.get_map_coalescing_objs(obj);

  if (!data.has_value()) {
    return false;
  }

  map_objs = data.value();
  return true;
}

bool get_map_coalescing_objs_from_map_op(const EP *ep, const Call *map_op,
                                         map_coalescing_objs_t &map_objs) {
  const call_t &call = map_op->get_call();

  assert(call.args.find("map") != call.args.end() && "No map argument");
  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const Context &ctx = ep->get_ctx();
  std::optional<map_coalescing_objs_t> data = ctx.get_map_coalescing_objs(obj);

  if (!data.has_value()) {
    return false;
  }

  map_objs = data.value();
  return true;
}

std::vector<int> get_past_recirculations(const EPNode *node) {
  std::vector<int> past_recirculations;

  while ((node = node->get_prev())) {
    const Module *module = node->get_module();

    if (!module) {
      continue;
    }

    if (module->get_type() == ModuleType::Tofino_Recirculate) {
      const tofino::Recirculate *recirc_module = dynamic_cast<const tofino::Recirculate *>(module);
      past_recirculations.push_back(recirc_module->get_recirc_port());
    }
  }

  return past_recirculations;
}

bool forwarding_decision_already_made(const EPNode *node) {
  while ((node = node->get_prev())) {
    const Module *module = node->get_module();

    if (!module) {
      continue;
    }

    if (module->get_type() == ModuleType::Tofino_Forward ||
        module->get_type() == ModuleType::Tofino_Drop ||
        module->get_type() == ModuleType::Tofino_Broadcast) {
      return true;
    }
  }

  return false;
}

port_ingress_t get_node_egress(const EP *ep, const EPNode *node) {
  const Context &ctx = ep->get_ctx();
  const Profiler &profiler = ctx.get_profiler();

  port_ingress_t egress;
  hit_rate_t hr = profiler.get_hr(node);

  if (node->get_module()->get_target() == TargetType::Controller) {
    egress.controller = hr;
    return egress;
  }

  std::vector<int> past_recirculations = get_past_recirculations(node);

  if (past_recirculations.empty()) {
    egress.global = hr;
  } else {
    int rport = past_recirculations[0];
    int depth = 1;

    for (size_t i = 1; i < past_recirculations.size(); i++) {
      if (past_recirculations[i] == rport) {
        depth++;
      } else {
        break;
      }
    }

    egress.recirc[{rport, depth}] = hr;
  }

  return egress;
}

void delete_all_vector_key_operations_from_bdd(BDD *bdd) {
  // 1. Get all map operations.
  // 2. Remove all vector operations storing the key.
  std::unordered_set<addr_t> maps;

  bdd->get_root()->visit_nodes([&maps](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_get" && call.function_name != "map_put" &&
        call.function_name != "map_erase") {
      return NodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    maps.insert(obj);

    return NodeVisitAction::Continue;
  });

  // There are more efficient ways of doing this (that don't involve traversing
  // the entire BDD every single time), but this is a quick and dirty way of
  // doing it.
  for (addr_t map : maps) {
    delete_all_vector_key_operations_from_bdd(bdd, map);
  }
}
} // namespace synapse