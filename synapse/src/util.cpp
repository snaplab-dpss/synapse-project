#include <algorithm>

#include "util.h"
#include "bdd/bdd.h"
#include "targets/targets.h"
#include "execution_plan/execution_plan.h"
#include "exprs/retriever.h"
#include "exprs/simplifier.h"
#include "exprs/solver.h"

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

bool is_incrementing_op(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after) {
  if (after->getKind() != klee::Expr::Add) {
    return false;
  }

  klee::ref<klee::Expr> lhs = after->getKid(0);
  klee::ref<klee::Expr> rhs = after->getKid(1);

  bool lhs_is_const = lhs->getKind() == klee::Expr::Constant;
  bool rhs_is_const = rhs->getKind() == klee::Expr::Constant;

  if (!lhs_is_const && !rhs_is_const) {
    return false;
  }

  klee::ref<klee::Expr> const_expr = lhs_is_const ? lhs : rhs;
  klee::ref<klee::Expr> not_const_expr = lhs_is_const ? rhs : lhs;

  // We only support increment of one, for now...
  ASSERT(solver_toolbox.value_from_expr(const_expr) == 1, "Expecting increment");

  return solver_toolbox.are_exprs_always_equal(not_const_expr, before);
}

std::optional<u64> get_max_value(klee::ref<klee::Expr> original_value,
                                 klee::ref<klee::Expr> condition) {
  std::optional<u64> max_value;

  std::optional<std::string> original_symbol = get_symbol(original_value);
  ASSERT(original_symbol.has_value(), "Expecting a symbol");

  std::optional<std::string> symbol = get_symbol(condition);

  if (!symbol.has_value()) {
    return max_value;
  }

  // We are looking for expression that look like this:
  // !(65536 <= vector_data_reset)
  // We should be more careful with this and be compatible with more
  // expressions.

  while (condition->getKind() == klee::Expr::Not) {
    condition = condition->getKid(0);
  }

  if (condition->getKind() == klee::Expr::Eq) {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    if (lhs->getKind() != klee::Expr::Constant) {
      return max_value;
    }

    u64 const_value = solver_toolbox.value_from_expr(lhs);

    if (const_value != 0) {
      return max_value;
    }

    condition = rhs;
  }

  if (condition->getKind() != klee::Expr::Ule) {
    return max_value;
  }

  klee::ref<klee::Expr> lhs = condition->getKid(0);
  klee::ref<klee::Expr> rhs = condition->getKid(1);

  bool lhs_is_const = lhs->getKind() == klee::Expr::Constant;
  bool rhs_is_const = rhs->getKind() == klee::Expr::Constant;

  if (!lhs_is_const && !rhs_is_const) {
    return max_value;
  }

  klee::ref<klee::Expr> const_expr = lhs_is_const ? lhs : rhs;
  klee::ref<klee::Expr> not_const_expr = lhs_is_const ? rhs : lhs;

  if (!solver_toolbox.are_exprs_always_equal(not_const_expr, original_value)) {
    return max_value;
  }

  return solver_toolbox.value_from_expr(const_expr);
}

bool is_counter_inc_op(const Node *vector_borrow, std::optional<u64> &max_value) {
  const Node *branch = vector_borrow->get_next();
  const Branch *branch_node = dynamic_cast<const Branch *>(branch);

  if (!branch_node) {
    return false;
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  const Node *on_true = branch_node->get_on_true();
  const Node *on_false = branch_node->get_on_false();

  const Call *borrow_node = dynamic_cast<const Call *>(vector_borrow);
  const Call *on_true_node = dynamic_cast<const Call *>(on_true);
  const Call *on_false_node = dynamic_cast<const Call *>(on_false);

  if (!on_true_node || !on_false_node) {
    return false;
  }

  const call_t &borrow_call = borrow_node->get_call();
  const call_t &on_true_call = on_true_node->get_call();
  const call_t &on_false_call = on_false_node->get_call();

  if (on_true_call.function_name != "vector_return" ||
      on_false_call.function_name != "vector_return") {
    return false;
  }

  klee::ref<klee::Expr> borrow_vector = borrow_call.args.at("vector").expr;
  klee::ref<klee::Expr> on_true_vector = on_true_call.args.at("vector").expr;
  klee::ref<klee::Expr> on_false_vector = on_false_call.args.at("vector").expr;

  addr_t borrow_vector_addr = expr_addr_to_obj_addr(borrow_vector);
  addr_t on_true_vector_addr = expr_addr_to_obj_addr(on_true_vector);
  addr_t on_false_vector_addr = expr_addr_to_obj_addr(on_false_vector);

  if (borrow_vector_addr != on_true_vector_addr ||
      borrow_vector_addr != on_false_vector_addr) {
    return false;
  }

  klee::ref<klee::Expr> borrow_value = borrow_call.extra_vars.at("borrowed_cell").second;
  klee::ref<klee::Expr> on_true_value = on_true_call.args.at("value").in;
  klee::ref<klee::Expr> on_false_value = on_false_call.args.at("value").in;

  bool on_true_inc_op = is_incrementing_op(borrow_value, on_true_value);

  if (!on_true_inc_op) {
    return false;
  }

  bool on_false_eq = solver_toolbox.are_exprs_always_equal(borrow_value, on_false_value);

  if (!on_false_eq) {
    return false;
  }

  std::optional<u64> local_max_value = get_max_value(borrow_value, condition);
  ASSERT(local_max_value.has_value(), "Expecting a max value for counter.");

  if (!max_value.has_value()) {
    max_value = local_max_value;
  } else if (*max_value != *local_max_value) {
    return false;
  }

  return true;
}

bool is_counter_read_op(const Node *vector_borrow) {
  const Node *vector_return = vector_borrow->get_next();

  const Call *borrow_node = dynamic_cast<const Call *>(vector_borrow);
  const Call *return_node = dynamic_cast<const Call *>(vector_return);

  if (!return_node) {
    return false;
  }

  const call_t &borrow_call = borrow_node->get_call();
  const call_t &return_call = return_node->get_call();

  if (return_call.function_name != "vector_return") {
    return false;
  }

  klee::ref<klee::Expr> borrow_vector = borrow_call.args.at("vector").expr;
  klee::ref<klee::Expr> return_vector = return_call.args.at("vector").expr;

  addr_t borrow_vector_addr = expr_addr_to_obj_addr(borrow_vector);
  addr_t return_vector_addr = expr_addr_to_obj_addr(return_vector);

  if (borrow_vector_addr != return_vector_addr) {
    return false;
  }

  klee::ref<klee::Expr> borrow_value = borrow_call.extra_vars.at("borrowed_cell").second;
  klee::ref<klee::Expr> return_value = return_call.args.at("value").in;

  bool equal_values = solver_toolbox.are_exprs_always_equal(borrow_value, return_value);

  return equal_values;
}

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

next_t get_allowed_coalescing_objs(std::vector<const Call *> index_allocators,
                                   addr_t obj) {
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

Branch *create_new_branch(BDD *bdd, const Node *current,
                          klee::ref<klee::Expr> condition) {
  node_id_t &id = bdd->get_mutable_id();
  NodeManager &manager = bdd->get_mutable_manager();
  klee::ConstraintManager constraints = current->get_constraints();
  Branch *new_branch = new Branch(id++, constraints, condition);
  manager.add_node(new_branch);
  return new_branch;
}

std::unordered_set<std::string> get_allowed_symbols_for_index_alloc_checking(
    const symbol_t &out_of_space,
    const std::optional<expiration_data_t> &expiration_data) {
  std::unordered_set<std::string> allowed_symbols;

  allowed_symbols.insert(out_of_space.array->name);

  if (expiration_data.has_value()) {
    allowed_symbols.insert(expiration_data->number_of_freed_flows.array->name);
  }

  return allowed_symbols;
}

std::vector<const Call *>
get_unfiltered_coalescing_nodes(const BDD *bdd, const Node *node,
                                const map_coalescing_objs_t &data) {
  std::vector<std::string> target_functions = {
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

  std::vector<const Call *> unfiltered_nodes =
      get_future_functions(node, target_functions);

  auto filter_map_objs = [&data](const Node *node) {
    ASSERT(node->get_type() == NodeType::Call, "Unexpected node type");

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

addr_t get_vector_map_key(const BDD *bdd,
                          const map_coalescing_objs_t &map_coalescing_objs) {
  addr_t vector_key;

  std::vector<const Call *> vector_borrows =
      get_future_functions(bdd->get_root(), {"vector_borrow"});

  for (const Call *vector_borrow : vector_borrows) {
    const call_t &vb = vector_borrow->get_call();

    klee::ref<klee::Expr> vector_expr = vb.args.at("vector").expr;
    klee::ref<klee::Expr> value_addr_expr = vb.args.at("val_out").out;

    addr_t vector_obj = expr_addr_to_obj_addr(vector_expr);
    addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    if (map_coalescing_objs.vectors.find(vector_obj) ==
        map_coalescing_objs.vectors.end()) {
      continue;
    }

    std::vector<const Call *> map_puts = get_future_functions(vector_borrow, {"map_put"});

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

    const Call *vector_return = get_future_vector_return(vector_borrow);
    const call_t &vr = vector_return->get_call();

    klee::ref<klee::Expr> ret_vector_expr = vr.args.at("vector").expr;
    klee::ref<klee::Expr> ret_value_addr_expr = vr.args.at("value").expr;

    addr_t ret_vector_obj = expr_addr_to_obj_addr(vector_expr);
    addr_t ret_value_addr = expr_addr_to_obj_addr(value_addr_expr);

    ASSERT(ret_vector_obj == vector_obj, "Vector objects don't match");

    // vector_borrow operation deemed this vector as a key vector, but the
    // vector_return contradicts this information.
    ASSERT(ret_value_addr == value_addr, "Value addresses don't match");

    return vector_obj;
  }

  PANIC("Vector key not found");
}

void delete_all_vector_key_operations_from_bdd(BDD *bdd, addr_t map) {
  map_coalescing_objs_t map_coalescing_data;
  if (!get_map_coalescing_objs_from_bdd(bdd, map, map_coalescing_data)) {
    return;
  }

  addr_t vector_key = get_vector_map_key(bdd, map_coalescing_data);
  std::unordered_set<const Node *> candidates;

  bdd->get_root()->visit_nodes([map_coalescing_data, vector_key,
                                &candidates](const Node *node) {
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
    delete_non_branch_node_from_bdd(bdd, node->get_id());
  }
}
} // namespace

pps_t bps2pps(bps_t bps, bytes_t pkt_size) {
  return bps / ((PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8);
}

bps_t pps2bps(pps_t pps, bytes_t pkt_size) {
  return pps * (PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8;
}

bool check_same_obj(const Call *call0, const Call *call1, const std::string &obj_name) {
  const call_t &c0 = call0->get_call();
  const call_t &c1 = call1->get_call();

  auto it0 = c0.args.find(obj_name);
  auto it1 = c1.args.find(obj_name);

  if (it0 == c0.args.end() || it1 == c1.args.end()) {
    return false;
  }

  klee::ref<klee::Expr> obj0 = it0->second.expr;
  klee::ref<klee::Expr> obj1 = it1->second.expr;

  return solver_toolbox.are_exprs_always_equal(obj0, obj1);
}

std::unordered_set<std::string> get_symbols(const Node *node) {
  switch (node->get_type()) {
  case NodeType::Branch: {
    const Branch *branch_node = dynamic_cast<const Branch *>(node);
    return get_symbols(branch_node->get_condition());
  };
  case NodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    std::unordered_set<std::string> symbols;

    for (const auto &[arg_name, arg] : call.args) {
      if (!arg.expr.isNull()) {
        std::unordered_set<std::string> s = get_symbols(arg.expr);
        symbols.insert(s.begin(), s.end());
      }

      if (!arg.in.isNull()) {
        std::unordered_set<std::string> s = get_symbols(arg.in);
        symbols.insert(s.begin(), s.end());
      }

      if (!arg.out.isNull()) {
        std::unordered_set<std::string> s = get_symbols(arg.out);
        symbols.insert(s.begin(), s.end());
      }
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      if (!extra_var.first.isNull()) {
        std::unordered_set<std::string> s = get_symbols(extra_var.first);
        symbols.insert(s.begin(), s.end());
      }

      if (!extra_var.second.isNull()) {
        std::unordered_set<std::string> s = get_symbols(extra_var.second);
        symbols.insert(s.begin(), s.end());
      }
    }

    return symbols;
  };
  default:
    return {};
  }
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

std::vector<mod_t> build_vector_modifications(const Call *vector_borrow,
                                              const Call *vector_return) {
  using vectors_pair = std::pair<node_id_t, node_id_t>;
  using cache_t = std::unordered_map<vectors_pair, std::vector<mod_t>, pair_hash>;
  static cache_t cache;

  auto cache_found_it = cache.find({vector_borrow->get_id(), vector_return->get_id()});
  if (cache_found_it != cache.end()) {
    return cache_found_it->second;
  }

  const call_t &vb_call = vector_borrow->get_call();
  const call_t &vr_call = vector_return->get_call();

  ASSERT(vb_call.function_name == "vector_borrow", "Unexpected function");
  ASSERT(vr_call.function_name == "vector_return", "Unexpected function");

  klee::ref<klee::Expr> original_value = vb_call.extra_vars.at("borrowed_cell").second;
  klee::ref<klee::Expr> value = vr_call.args.at("value").in;

  std::vector<mod_t> changes = build_expr_mods(original_value, value);

  cache[{vector_borrow->get_id(), vector_return->get_id()}] = changes;

  return changes;
}

std::vector<mod_t> build_hdr_modifications(const Call *packet_borrow_next_chunk,
                                           const Call *packet_return_chunk) {
  static std::unordered_map<node_id_t, std::vector<mod_t>> cache;

  auto cache_found_it = cache.find(packet_return_chunk->get_id());
  if (cache_found_it != cache.end()) {
    return cache_found_it->second;
  }

  const call_t &ret_call = packet_return_chunk->get_call();
  ASSERT(ret_call.function_name == "packet_return_chunk", "Unexpected function");

  const call_t &bor_call = packet_borrow_next_chunk->get_call();
  ASSERT(bor_call.function_name == "packet_borrow_next_chunk", "Unexpected function");

  klee::ref<klee::Expr> borrowed = bor_call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> returned = ret_call.args.at("the_chunk").in;
  ASSERT(borrowed->getWidth() == returned->getWidth(), "Different widths");

  std::vector<mod_t> changes = build_expr_mods(borrowed, returned);

  cache[packet_return_chunk->get_id()] = changes;

  return changes;
}

// This is somewhat of a hack... We assume that checksum expressions will only
// be used to modify checksum fields of a packet, not other packet fields.
std::vector<mod_t>
ignore_checksum_modifications(const std::vector<mod_t> &modifications) {
  std::vector<mod_t> filtered;

  for (const mod_t &mod : modifications) {
    klee::ref<klee::Expr> simplified = simplify(mod.expr);
    std::unordered_set<std::string> symbols = get_symbols(simplified);

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

bool query_contains_map_has_key(const Branch *node) {
  ASSERT(!node->get_condition().isNull(), "No condition");

  klee::ref<klee::Expr> _condition = node->get_condition();
  std::unordered_set<std::string> symbols = get_symbols(_condition);

  auto found_it =
      std::find_if(symbols.begin(), symbols.end(), [](const std::string &symbol) -> bool {
        return symbol.find("map_has_this_key") != std::string::npos;
      });

  if (found_it == symbols.end()) {
    return false;
  }

  return true;
}

const Call *packet_borrow_from_return(const EP *ep, const Call *packet_return_chunk) {
  const call_t &call = packet_return_chunk->get_call();
  ASSERT(call.function_name == "packet_return_chunk", "Unexpected function");

  klee::ref<klee::Expr> chunk_returned = call.args.at("the_chunk").in;

  std::vector<const Call *> prev_borrows =
      get_prev_functions(ep, packet_return_chunk, {"packet_borrow_next_chunk"});

  std::vector<const Call *> prev_returns =
      get_prev_functions(ep, packet_return_chunk, {"packet_return_chunk"});

  ASSERT(prev_borrows.size(), "No previous borrows");
  ASSERT(prev_borrows.size() > prev_returns.size(), "No previous borrow");

  return prev_borrows[prev_borrows.size() - 1 - prev_returns.size()];
}

std::vector<const Call *> get_prev_functions(const EP *ep, const Node *node,
                                             const std::vector<std::string> &fnames,
                                             bool ignore_targets) {
  std::vector<const Call *> prev_functions;

  TargetType target = ep->get_active_target();
  const nodes_t &roots = ep->get_target_roots(target);

  if (!node) {
    return prev_functions;
  }

  while ((node = node->get_prev())) {
    if (node->get_type() == NodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call = call_node->get_call();
      const std::string &fname = call.function_name;

      auto found_it = std::find(fnames.begin(), fnames.end(), fname);
      if (found_it != fnames.end()) {
        prev_functions.insert(prev_functions.begin(), call_node);
      }
    }

    if (!ignore_targets && roots.find(node->get_id()) != roots.end()) {
      break;
    }
  }

  return prev_functions;
}

std::vector<const Call *> get_future_functions(const Node *root,
                                               const std::vector<std::string> &wanted,
                                               bool stop_on_branches) {
  std::vector<const Call *> functions;

  root->visit_nodes([&functions, &wanted](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    auto found_it = std::find(wanted.begin(), wanted.end(), call.function_name);

    if (found_it != wanted.end()) {
      functions.push_back(call_node);
    }

    return NodeVisitAction::Continue;
  });

  return functions;
}

bool is_parser_drop(const Node *root) {
  bool found_drop = false;

  root->visit_nodes([&found_drop](const Node *node) {
    if (node->get_type() == NodeType::Branch) {
      return NodeVisitAction::Stop;
    }

    if (node->get_type() != NodeType::Route) {
      return NodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(node);
    RouteOp op = route_node->get_operation();

    found_drop = (op == RouteOp::Drop);
    return NodeVisitAction::Stop;
  });

  return found_drop;
}

std::vector<const Module *> get_prev_modules(const EP *ep,
                                             const std::vector<ModuleType> &targets) {
  std::vector<const Module *> modules;

  EPLeaf leaf = ep->get_active_leaf();
  const EPNode *ep_node = leaf.node;

  while (ep_node) {
    const EPNode *current = ep_node;
    ep_node = current->get_prev();

    const Module *module = current->get_module();
    ModuleType type = module->get_type();

    auto found_it = std::find(targets.begin(), targets.end(), type);
    if (found_it != targets.end()) {
      modules.push_back(module);
    }
  }

  return modules;
}

bool is_expr_only_packet_dependent(klee::ref<klee::Expr> expr) {
  std::unordered_set<std::string> symbols = get_symbols(expr);

  std::vector<std::string> allowed_symbols = {
      "pkt_len",
      "packet_chunks",
      "DEVICE",
  };

  for (auto symbol : symbols) {
    auto found_it = std::find(allowed_symbols.begin(), allowed_symbols.end(), symbol);

    if (found_it == allowed_symbols.end()) {
      return false;
    }
  }

  return true;
}

counter_data_t is_counter(const EP *ep, addr_t obj) {
  counter_data_t data;

  const BDD *bdd = ep->get_bdd();
  vector_config_t cfg = get_vector_config_from_bdd(*bdd, obj);

  if (cfg.elem_size > 64 || cfg.capacity != 1) {
    return data;
  }

  const Node *root = bdd->get_root();
  std::vector<const Call *> vector_borrows =
      get_future_functions(root, {"vector_borrow"});

  for (const Call *vector_borrow : vector_borrows) {
    const call_t &call = vector_borrow->get_call();

    auto _vector = call.args.at("vector").expr;
    auto _vector_addr = expr_addr_to_obj_addr(_vector);

    if (_vector_addr != obj) {
      continue;
    }

    if (is_counter_read_op(vector_borrow)) {
      data.reads.push_back(vector_borrow);
      continue;
    }

    if (is_counter_inc_op(vector_borrow, data.max_value)) {
      data.writes.push_back(vector_borrow);
      continue;
    }

    return data;
  }

  data.valid = true;
  return data;
}

klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node,
                                                addr_t target_addr) {
  const Node *source;
  return get_original_vector_value(ep, node, target_addr, source);
}

klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node,
                                                addr_t target_addr, const Node *&source) {
  std::vector<const Call *> all_prev_vector_borrow =
      get_prev_functions(ep, node, {"vector_borrow"}, true);

  klee::ref<klee::Expr> borrowed_cell;

  for (const Call *prev_vector_borrow : all_prev_vector_borrow) {
    const call_t &call = prev_vector_borrow->get_call();

    klee::ref<klee::Expr> _vector = call.args.at("vector").expr;
    klee::ref<klee::Expr> _borrowed_cell = call.extra_vars.at("borrowed_cell").second;

    addr_t _vector_addr = expr_addr_to_obj_addr(_vector);

    if (_vector_addr != target_addr) {
      continue;
    }

    source = prev_vector_borrow;
    borrowed_cell = _borrowed_cell;
    break;
  }

  ASSERT(!borrowed_cell.isNull(), "Expecting a previous vector borrow but not found.");

  return borrowed_cell;
}

const Call *get_future_vector_return(const Call *vector_borrow) {
  std::vector<const Call *> found;

  const call_t &vb_call = vector_borrow->get_call();
  klee::ref<klee::Expr> target_addr_expr = vb_call.args.at("vector").expr;

  addr_t target_addr = expr_addr_to_obj_addr(target_addr_expr);
  klee::ref<klee::Expr> target_index = vb_call.args.at("index").expr;

  for (const Call *vector_return :
       get_future_functions(vector_borrow, {"vector_return"})) {
    const call_t &call = vector_return->get_call();

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t obj = expr_addr_to_obj_addr(vector_addr_expr);

    if (obj != target_addr) {
      continue;
    }

    if (!solver_toolbox.are_exprs_always_equal(index, target_index)) {
      continue;
    }

    return vector_return;
  }

  return nullptr;
}

klee::ref<klee::Expr> get_expr_from_addr(const EP *ep, addr_t addr) {
  const BDD *bdd = ep->get_bdd();
  const Node *root = bdd->get_root();

  std::vector<const Call *> nodes = get_future_functions(root, {"map_get"});

  for (const Call *node : nodes) {
    const call_t &call = node->get_call();

    klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    addr_t key_addr_value = expr_addr_to_obj_addr(key_addr);

    if (key_addr_value != addr) {
      continue;
    }

    return key;
  }

  return nullptr;
}

bool get_map_coalescing_objs_from_bdd(const BDD *bdd, addr_t obj,
                                      map_coalescing_objs_t &data) {
  const Node *root = bdd->get_root();

  std::vector<const Call *> index_allocators =
      get_future_functions(root, {"dchain_allocate_new_index"});

  if (index_allocators.empty()) {
    return false;
  }

  next_t candidates = get_allowed_coalescing_objs(index_allocators, obj);

  if (candidates.size() == 0) {
    return false;
  }

  ASSERT(candidates.maps.size() == 1, "Expecting a single map");
  ASSERT(candidates.dchains.size() == 1, "Expecting a single dchain");

  data.map = candidates.maps.begin()->obj;
  data.dchain = candidates.dchains.begin()->obj;

  for (const auto &vector : candidates.vectors) {
    data.vectors.insert(vector.obj);
  }

  return true;
}

bool is_parser_condition(const Branch *branch) {
  std::vector<const Call *> future_borrows =
      get_future_functions(branch, {"packet_borrow_next_chunk"});

  if (future_borrows.size() == 0) {
    return false;
  }

  klee::ref<klee::Expr> condition = branch->get_condition();
  bool only_looks_at_packet = is_expr_only_packet_dependent(condition);

  return only_looks_at_packet;
}

klee::ref<klee::Expr> get_chunk_from_borrow(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return nullptr;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return nullptr;
  }

  return call.extra_vars.at("the_chunk").second;
}

bool borrow_has_var_len(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return false;
  }

  klee::ref<klee::Expr> length = call.args.at("length").expr;
  return length->getKind() != klee::Expr::Kind::Constant;
}

symbols_t get_prev_symbols(const Node *node, const nodes_t &stop_nodes) {
  symbols_t symbols;

  ASSERT(node, "Node is null");
  node = node->get_prev();

  std::unordered_set<std::string> ignoring_symbols = {
      "packet_chunks",
      "out_of_space",
  };

  while (node) {
    if (stop_nodes.find(node->get_id()) != stop_nodes.end()) {
      break;
    }

    if (node->get_type() == NodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(node);
      symbols_t local_symbols = call_node->get_locally_generated_symbols();

      for (const symbol_t &symbol : local_symbols) {
        if (ignoring_symbols.find(symbol.base) != ignoring_symbols.end()) {
          continue;
        }

        symbols.insert(symbol);
      }
    }

    node = node->get_prev();
  }

  return symbols;
}

bool is_vector_return_without_modifications(const EP *ep, const Node *node) {
  static std::unordered_map<node_id_t, bool> cache;

  auto found_cache_it = cache.find(node->get_id());
  if (found_cache_it != cache.end()) {
    return found_cache_it->second;
  }

  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_return") {
    return false;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> value = call.args.at("value").in;

  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  klee::ref<klee::Expr> original_value =
      get_original_vector_value(ep, call_node, vector_addr);
  std::vector<mod_t> changes = build_expr_mods(original_value, value);

  bool veredict = changes.empty();
  cache[node->get_id()] = veredict;

  return veredict;
}

bool is_vector_read(const Call *vector_borrow) {
  const call_t &vb = vector_borrow->get_call();

  if (vb.function_name != "vector_borrow") {
    return false;
  }

  klee::ref<klee::Expr> vb_obj_expr = vb.args.at("vector").expr;
  klee::ref<klee::Expr> vb_index = vb.args.at("index").expr;
  klee::ref<klee::Expr> vb_value = vb.extra_vars.at("borrowed_cell").second;

  addr_t vb_obj = expr_addr_to_obj_addr(vb_obj_expr);

  const Call *vector_return = get_future_vector_return(vector_borrow);

  if (!vector_return) {
    return true;
  }

  ASSERT(vector_return->get_type() == NodeType::Call, "Unexpected node type");

  const Call *vr_call = dynamic_cast<const Call *>(vector_return);
  const call_t &vr = vr_call->get_call();
  ASSERT(vr.function_name == "vector_return", "Unexpected function");

  klee::ref<klee::Expr> vr_obj_expr = vr.args.at("vector").expr;
  klee::ref<klee::Expr> vr_index = vr.args.at("index").expr;
  klee::ref<klee::Expr> vr_value = vr.args.at("value").in;

  addr_t vr_obj = expr_addr_to_obj_addr(vr_obj_expr);
  ASSERT(vb_obj == vr_obj, "Different objects");
  ASSERT(solver_toolbox.are_exprs_always_equal(vb_index, vr_index), "Different indexes");

  return solver_toolbox.are_exprs_always_equal(vb_value, vr_value);
}

bool is_vector_write(const Call *call_node) {
  if (call_node->get_call().function_name != "vector_borrow") {
    return false;
  }

  return !is_vector_read(call_node);
}

bool is_vector_borrow_ignored(const Call *vector_borrow) {
  static std::unordered_map<node_id_t, bool> cache;

  if (vector_borrow->get_call().function_name != "vector_borrow") {
    return false;
  }

  if (!vector_borrow->get_next()) {
    return true;
  }

  auto found_cache_it = cache.find(vector_borrow->get_id());
  if (found_cache_it != cache.end()) {
    return found_cache_it->second;
  }

  if (!is_vector_read(vector_borrow)) {
    cache[vector_borrow->get_id()] = false;
    return false;
  }

  symbols_t symbols = vector_borrow->get_locally_generated_symbols();
  symbol_t value;
  bool found = get_symbol(symbols, "vector_data_reset", value);
  ASSERT(found, "Symbol vector_data_reset not found");

  bool used = false;

  vector_borrow->get_next()->visit_nodes([&value, &used](const Node *node) {
    for (const auto &symbol_label : get_symbols(node)) {
      if (symbol_label == value.array->name) {
        used = true;
        return NodeVisitAction::Stop;
      }
    }

    return NodeVisitAction::Continue;
  });

  cache[vector_borrow->get_id()] = !used;

  return !used;
}

branch_direction_t get_map_get_success_check(const Node *node) {
  ASSERT(node->get_type() == NodeType::Call, "Unexpected node type");
  const Call *map_get = dynamic_cast<const Call *>(node);

  const call_t &mg_call = map_get->get_call();
  ASSERT(mg_call.function_name == "map_get", "Unexpected function");

  klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  symbols_t symbols = map_get->get_locally_generated_symbols();
  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  klee::ref<klee::Expr> key_not_found_cond = solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  branch_direction_t success_check = {nullptr, false};

  map_get->visit_nodes([&success_check, key_not_found_cond, obj, key](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch = dynamic_cast<const Branch *>(node);
    klee::ref<klee::Expr> condition = branch->get_condition();

    bool is_key_not_found_cond =
        solver_toolbox.are_exprs_always_equal(condition, key_not_found_cond);
    bool is_not_key_not_found_cond = solver_toolbox.are_exprs_always_equal(
        solver_toolbox.exprBuilder->Not(condition), key_not_found_cond);

    if (!is_key_not_found_cond && !is_not_key_not_found_cond) {
      return NodeVisitAction::SkipChildren;
    }

    success_check.branch = branch;
    success_check.direction = !is_key_not_found_cond;

    return NodeVisitAction::Stop;
  });

  return success_check;
}

rw_fractions_t get_cond_map_put_rw_profile_fractions(const EP *ep, const Node *node) {
  const Context &ctx = ep->get_ctx();
  const Profiler &profiler = ctx.get_profiler();

  ASSERT(node->get_type() == NodeType::Call, "Unexpected node type");
  const Call *map_get = dynamic_cast<const Call *>(node);

  const call_t &mg_call = map_get->get_call();
  ASSERT(mg_call.function_name == "map_get", "Unexpected function");

  klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  symbols_t symbols = map_get->get_locally_generated_symbols();
  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  rw_fractions_t fractions;

  branch_direction_t success_check = get_map_get_success_check(map_get);
  ASSERT(success_check.branch, "Map get success check not found");

  const Node *read = success_check.direction ? success_check.branch->get_on_true()
                                             : success_check.branch->get_on_false();
  const Node *write_attempt = success_check.direction
                                  ? success_check.branch->get_on_false()
                                  : success_check.branch->get_on_true();

  std::vector<const Call *> future_map_puts =
      get_future_functions(write_attempt, {"map_put"});
  ASSERT(future_map_puts.size() >= 1, "map_put not found");

  const Node *write = nullptr;
  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    ASSERT(mp_call.function_name == "map_put", "Unexpected function");

    klee::ref<klee::Expr> o = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> k = mp_call.args.at("key").in;

    if (solver_toolbox.are_exprs_always_equal(o, obj) &&
        solver_toolbox.are_exprs_always_equal(k, key)) {
      write = map_put;
      break;
    }
  }

  ASSERT(write, "map_put not found");

  fractions.read = profiler.get_hr(read);
  fractions.write_attempt = profiler.get_hr(write_attempt);
  fractions.write = profiler.get_hr(write);

  return fractions;
}

bool is_map_get_followed_by_map_puts_on_miss(const BDD *bdd, const Call *map_get,
                                             std::vector<const Call *> &map_puts) {
  const call_t &mg_call = map_get->get_call();

  if (mg_call.function_name != "map_get") {
    return false;
  }

  klee::ref<klee::Expr> obj_expr = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  symbols_t symbols = map_get->get_locally_generated_symbols();
  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  klee::ref<klee::Expr> failed_map_get = solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  std::vector<const Call *> future_map_puts = get_future_functions(map_get, {"map_put"});

  klee::ref<klee::Expr> value;

  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    ASSERT(mp_call.function_name == "map_put", "Unexpected function");

    klee::ref<klee::Expr> map_expr = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> mp_key = mp_call.args.at("key").in;
    klee::ref<klee::Expr> mp_value = mp_call.args.at("value").expr;

    addr_t map = expr_addr_to_obj_addr(map_expr);

    if (map != obj) {
      continue;
    }

    if (!solver_toolbox.are_exprs_always_equal(key, mp_key)) {
      return false;
    }

    if (value.isNull()) {
      value = mp_value;
    } else if (!solver_toolbox.are_exprs_always_equal(value, mp_value)) {
      return false;
    }

    klee::ConstraintManager constraints = map_put->get_constraints();
    if (!solver_toolbox.is_expr_always_true(constraints, failed_map_get)) {
      // Found map_put that happens even if map_get was successful.
      return false;
    }

    map_puts.push_back(map_put);
  }

  if (map_puts.empty()) {
    return false;
  }

  return true;
}

bool are_map_read_write_counterparts(const Call *map_get, const Call *map_put) {
  const call_t &get = map_get->get_call();
  const call_t &put = map_put->get_call();

  if (get.function_name != "map_get" || put.function_name != "map_put") {
    return false;
  }

  klee::ref<klee::Expr> get_obj_expr = get.args.at("map").expr;
  klee::ref<klee::Expr> put_obj_expr = put.args.at("map").expr;

  addr_t get_obj = expr_addr_to_obj_addr(get_obj_expr);
  addr_t put_obj = expr_addr_to_obj_addr(put_obj_expr);

  if (get_obj != put_obj) {
    return false;
  }

  klee::ref<klee::Expr> get_key = get.args.at("key").in;
  klee::ref<klee::Expr> put_key = put.args.at("key").in;

  if (!solver_toolbox.are_exprs_always_equal(get_key, put_key)) {
    return false;
  }

  return true;
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

      if (!solver_toolbox.are_exprs_always_equal(call0_extra_var.first,
                                                 call1_extra_var.first)) {
        return false;
      }

      if (!solver_toolbox.are_exprs_always_equal(call0_extra_var.second,
                                                 call1_extra_var.second)) {
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
  map_rw_pattern.map_get_success_check = get_map_get_success_check(map_get);

  if (!map_rw_pattern.map_get_success_check.branch ||
      map_get->get_next() != map_rw_pattern.map_get_success_check.branch) {
    cache[map_get] = std::nullopt;
    return false;
  }

  klee::ref<klee::Expr> obj_expr = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;
  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  // 2. Checking for an extra condition before performing the map_put.

  const Node *on_failed_map_get =
      map_rw_pattern.map_get_success_check.direction
          ? map_rw_pattern.map_get_success_check.branch->get_on_false()
          : map_rw_pattern.map_get_success_check.branch->get_on_true();

  if (on_failed_map_get->get_type() == NodeType::Branch) {
    const Branch *write_extra_condition = dynamic_cast<const Branch *>(on_failed_map_get);

    const Node *on_true = write_extra_condition->get_on_true();
    const Node *on_false = write_extra_condition->get_on_false();

    std::vector<const Call *> on_true_mp = get_future_functions(on_true, {"map_put"});
    std::vector<const Call *> on_false_mp = get_future_functions(on_false, {"map_put"});

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

  map_rw_pattern.dchain_allocate_new_index =
      dynamic_cast<const Call *>(on_failed_map_get);

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
      find_branch_checking_index_alloc(ep, map_rw_pattern.dchain_allocate_new_index);

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

  const Node *on_index_alloc =
      map_rw_pattern.index_alloc_check.direction
          ? map_rw_pattern.index_alloc_check.branch->get_on_true()
          : map_rw_pattern.index_alloc_check.branch->get_on_false();
  const Node *on_failed_index_alloc =
      map_rw_pattern.index_alloc_check.direction
          ? map_rw_pattern.index_alloc_check.branch->get_on_false()
          : map_rw_pattern.index_alloc_check.branch->get_on_true();

  map_rw_pattern.map_put = nullptr;

  for (const Call *map_put : get_future_functions(on_index_alloc, {"map_put"})) {
    if (!are_map_read_write_counterparts(map_get, map_put)) {
      continue;
    }

    if (map_rw_pattern.map_put) {
      // Multiple map_put operations found.
      cache[map_get] = std::nullopt;
      return false;
    }

    map_rw_pattern.map_put = map_put;
  }

  for (const Call *map_put : get_future_functions(on_failed_index_alloc, {"map_put"})) {
    if (are_map_read_write_counterparts(map_get, map_put)) {
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

  klee::ref<klee::Expr> is_tracing_condition = solver_toolbox.exprBuilder->Ne(
      is_tracing_call.ret,
      solver_toolbox.exprBuilder->Constant(0, is_tracing_call.ret->getWidth()));

  std::vector<const Call *> tb_update_and_checks =
      get_future_functions(tb_is_tracing, {"tb_update_and_check"});

  tb_update_and_check = nullptr;
  for (const Call *candidate : tb_update_and_checks) {
    if (!check_same_obj(tb_is_tracing, candidate, "tb")) {
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
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);

  if (!index_alloc_check.branch) {
    return false;
  }

  klee::ref<klee::Expr> condition = index_alloc_check.branch->get_condition();

  std::vector<const Call *> future_map_puts =
      get_future_functions(dchain_allocate_new_index, {"map_put"});

  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    ASSERT(mp_call.function_name == "map_put", "Unexpected function");

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

bool is_index_alloc_on_unsuccessful_map_get(const EP *ep,
                                            const Call *dchain_allocate_new_index) {
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

  symbol_t map_has_this_key;
  bool found =
      get_symbol(dynamic_cast<const Call *>(map_get)->get_locally_generated_symbols(),
                 "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  klee::ConstraintManager constraints = dchain_allocate_new_index->get_constraints();

  klee::ref<klee::Expr> found_key = solver_toolbox.exprBuilder->Ne(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  return solver_toolbox.is_expr_always_false(constraints, found_key);
}

bool is_map_get_followed_by_map_erases_on_hit(const BDD *bdd, const Call *map_get,
                                              std::vector<const Call *> &map_erases) {
  const call_t &mg_call = map_get->get_call();

  if (mg_call.function_name != "map_get") {
    return false;
  }

  klee::ref<klee::Expr> obj_expr = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  symbols_t symbols = map_get->get_locally_generated_symbols();
  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  klee::ref<klee::Expr> successful_map_get = solver_toolbox.exprBuilder->Ne(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  std::vector<const Call *> future_map_erases =
      get_future_functions(map_get, {"map_erase"});

  for (const Call *map_erase : future_map_erases) {
    const call_t &me_call = map_erase->get_call();
    ASSERT(me_call.function_name == "map_erase", "Unexpected function");

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

Node *add_non_branch_nodes_to_bdd(BDD *bdd, const Node *current,
                                  const std::vector<const Node *> &new_nodes) {
  Node *new_current = nullptr;

  if (new_nodes.empty()) {
    return new_current;
  }

  node_id_t &id = bdd->get_mutable_id();
  NodeManager &manager = bdd->get_mutable_manager();

  const Node *prev = current->get_prev();
  ASSERT(prev, "No previous node");

  node_id_t anchor_id = prev->get_id();
  Node *anchor = bdd->get_mutable_node_by_id(anchor_id);
  Node *anchor_next = bdd->get_mutable_node_by_id(current->get_id());

  bool set_new_current = false;

  for (const Node *new_node : new_nodes) {
    ASSERT(new_node->get_type() != NodeType::Branch, "Unexpected branch node");

    Node *clone = new_node->clone(manager, false);
    clone->recursive_update_ids(id);

    if (!set_new_current) {
      new_current = clone;
      set_new_current = true;
    }

    switch (anchor->get_type()) {
    case NodeType::Call:
    case NodeType::Route: {
      anchor->set_next(clone);
    } break;
    case NodeType::Branch: {
      Branch *branch = dynamic_cast<Branch *>(anchor);

      const Node *on_true = branch->get_on_true();
      const Node *on_false = branch->get_on_false();

      ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection found");

      if (on_true == anchor_next) {
        branch->set_on_true(clone);
      } else {
        branch->set_on_false(clone);
      }

    } break;
    }

    clone->set_prev(anchor);
    clone->set_next(anchor_next);
    anchor_next->set_prev(clone);
    anchor = clone;
  }

  return new_current;
}

Branch *add_branch_to_bdd(BDD *bdd, const Node *current,
                          klee::ref<klee::Expr> condition) {
  node_id_t &id = bdd->get_mutable_id();
  NodeManager &manager = bdd->get_mutable_manager();

  const Node *prev = current->get_prev();
  ASSERT(prev, "No previous node");

  node_id_t anchor_id = prev->get_id();
  Node *anchor = bdd->get_mutable_node_by_id(anchor_id);
  Node *anchor_next = bdd->get_mutable_node_by_id(current->get_id());

  klee::ref<klee::Expr> constraint = constraint_from_expr(condition);

  Node *on_true_cond = anchor_next;
  Node *on_false_cond = anchor_next->clone(manager, true);
  on_false_cond->recursive_update_ids(id);

  on_true_cond->recursive_add_constraint(constraint);
  on_false_cond->recursive_add_constraint(solver_toolbox.exprBuilder->Not(constraint));

  Branch *new_branch = create_new_branch(bdd, current, condition);

  new_branch->set_on_true(on_true_cond);
  new_branch->set_on_false(on_false_cond);

  on_true_cond->set_prev(new_branch);
  on_false_cond->set_prev(new_branch);

  switch (anchor->get_type()) {
  case NodeType::Call:
  case NodeType::Route: {
    anchor->set_next(new_branch);
  } break;
  case NodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection found");

    if (on_true == anchor_next) {
      branch->set_on_true(new_branch);
    } else {
      branch->set_on_false(new_branch);
    }

  } break;
  }

  new_branch->set_prev(anchor);

  return new_branch;
}

Node *delete_non_branch_node_from_bdd(BDD *bdd, node_id_t target_id) {
  NodeManager &manager = bdd->get_mutable_manager();

  Node *anchor_next = bdd->get_mutable_node_by_id(target_id);
  ASSERT(anchor_next, "Node not found");
  ASSERT(anchor_next->get_type() != NodeType::Branch, "Unexpected branch node");

  Node *anchor = anchor_next->get_mutable_prev();
  ASSERT(anchor, "No previous node");

  Node *new_current = anchor_next->get_mutable_next();

  switch (anchor->get_type()) {
  case NodeType::Call:
  case NodeType::Route: {
    anchor->set_next(new_current);
  } break;
  case NodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection");

    if (on_true == anchor_next) {
      branch->set_on_true(new_current);
    } else {
      branch->set_on_false(new_current);
    }

  } break;
  }

  new_current->set_prev(anchor);
  manager.free_node(anchor_next);

  return new_current;
}

Node *delete_branch_node_from_bdd(BDD *bdd, node_id_t target_id, bool direction_to_keep) {
  NodeManager &manager = bdd->get_mutable_manager();

  Node *target = bdd->get_mutable_node_by_id(target_id);
  ASSERT(target, "Node not found");
  ASSERT(target->get_type() == NodeType::Branch, "Unexpected branch node");

  Node *anchor = target->get_mutable_prev();
  ASSERT(anchor, "No previous node");

  Branch *anchor_next = dynamic_cast<Branch *>(target);

  Node *target_on_true = anchor_next->get_mutable_on_true();
  Node *target_on_false = anchor_next->get_mutable_on_false();

  Node *new_current;

  if (direction_to_keep) {
    new_current = target_on_true;
    target_on_false->recursive_free_children(manager);
    manager.free_node(target_on_false);
  } else {
    new_current = target_on_false;
    target_on_true->recursive_free_children(manager);
    manager.free_node(target_on_true);
  }

  switch (anchor->get_type()) {
  case NodeType::Call:
  case NodeType::Route: {
    anchor->set_next(new_current);
  } break;
  case NodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection");

    if (on_true == anchor_next) {
      branch->set_on_true(new_current);
    } else {
      branch->set_on_false(new_current);
    }

  } break;
  }

  new_current->set_prev(anchor);
  manager.free_node(anchor_next);

  return new_current;
}

branch_direction_t find_branch_checking_index_alloc(const EP *ep, const Node *node,
                                                    const symbol_t &out_of_space) {
  ASSERT(!out_of_space.expr.isNull(), "out_of_space expr is null");

  const Context &ctx = ep->get_ctx();
  const std::optional<expiration_data_t> expiration_data = ctx.get_expiration_data();

  std::unordered_set<std::string> target_symbols =
      get_allowed_symbols_for_index_alloc_checking(out_of_space, expiration_data);

  branch_direction_t index_alloc_check = {
      .branch = nullptr,
      .direction = false,
  };

  node->visit_nodes([&target_symbols, &index_alloc_check](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch = dynamic_cast<const Branch *>(node);

    klee::ref<klee::Expr> condition = branch->get_condition();
    std::unordered_set<std::string> used_symbols = get_symbols(condition);

    for (const std::string &symbol : used_symbols) {
      if (target_symbols.find(symbol) == target_symbols.end()) {
        return NodeVisitAction::Continue;
      }
    }

    index_alloc_check.branch = branch;
    return NodeVisitAction::Stop;
  });

  if (index_alloc_check.branch) {
    klee::ref<klee::Expr> success_condition = solver_toolbox.exprBuilder->Eq(
        out_of_space.expr,
        solver_toolbox.exprBuilder->Constant(0, out_of_space.expr->getWidth()));

    if (expiration_data.has_value()) {
      success_condition = solver_toolbox.exprBuilder->Or(
          success_condition,
          solver_toolbox.exprBuilder->Ne(
              expiration_data->number_of_freed_flows.expr,
              solver_toolbox.exprBuilder->Constant(
                  0, expiration_data->number_of_freed_flows.expr->getWidth())));
    }

    ASSERT(index_alloc_check.branch->get_on_true(), "No on_true");
    ASSERT(index_alloc_check.branch->get_on_false(), "No on_false");

    const Node *on_true = index_alloc_check.branch->get_on_true();
    const Node *on_false = index_alloc_check.branch->get_on_false();

    bool success_on_true =
        solver_toolbox.is_expr_always_true(on_true->get_constraints(), success_condition);
    bool success_on_false = solver_toolbox.is_expr_always_true(
        on_false->get_constraints(), success_condition);

    ASSERT((success_on_true || success_on_false), "No branch side is successful");
    ASSERT((success_on_true ^ success_on_false),
           "Both branch sides have the same success condition");

    index_alloc_check.direction = success_on_true;
  }

  return index_alloc_check;
}

branch_direction_t
find_branch_checking_index_alloc(const EP *ep, const Node *dchain_allocate_new_index) {
  ASSERT(dchain_allocate_new_index->get_type() == NodeType::Call, "Unexpected node type");
  const Call *call_node = dynamic_cast<const Call *>(dchain_allocate_new_index);
  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "dchain_allocate_new_index", "Unexpected function");

  symbol_t out_of_space;
  bool found = get_symbol(call_node->get_locally_generated_symbols(), "out_of_space",
                          out_of_space);
  ASSERT(found, "Symbol out_of_space not found");

  return find_branch_checking_index_alloc(ep, dchain_allocate_new_index, out_of_space);
}

std::vector<const Call *>
get_coalescing_nodes_from_key(const BDD *bdd, const Node *node,
                              klee::ref<klee::Expr> target_key,
                              const map_coalescing_objs_t &data) {
  std::vector<const Call *> filtered_nodes =
      get_unfiltered_coalescing_nodes(bdd, node, data);

  if (filtered_nodes.empty()) {
    return filtered_nodes;
  }

  klee::ref<klee::Expr> index;

  auto filter_map_nodes_and_retrieve_index = [&target_key, &index](const Node *node) {
    ASSERT(node->get_type() == NodeType::Call, "Unexpected node type");

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
    ASSERT(node->get_type() == NodeType::Call, "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return false;
    }

    ASSERT(!index.isNull(), "Index is null");

    klee::ref<klee::Expr> value = call.args.at("index").expr;
    return !solver_toolbox.are_exprs_always_equal(index, value);
  };

  filtered_nodes.erase(
      std::remove_if(filtered_nodes.begin(), filtered_nodes.end(), filter_vectors_nodes),
      filtered_nodes.end());

  return filtered_nodes;
}

symbol_t create_symbol(const std::string &label, bits_t size) {
  const klee::Array *array;

  klee::ref<klee::Expr> expr = solver_toolbox.create_new_symbol(label, size, array);

  symbol_t new_symbol = {
      .base = label,
      .array = array,
      .expr = expr,
  };

  return new_symbol;
}

std::string int2hr(u64 value) {
  std::stringstream ss;
  std::string str = std::to_string(value);

  // Add thousands separator
  for (int i = str.size() - 3; i > 0; i -= 3) {
    str.insert(i, THOUSANDS_SEPARATOR);
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
      str.insert(i, THOUSANDS_SEPARATOR);
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

  ASSERT(call.args.find("chain") != call.args.end(), "No chain argument");
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

  ASSERT(call.args.find("map") != call.args.end(), "No map argument");
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
      const tofino::Recirculate *recirc_module =
          dynamic_cast<const tofino::Recirculate *>(module);
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

  if (node->get_module()->get_target() == TargetType::TofinoCPU) {
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
