#include <LibBDD/BDD.h>
#include <LibBDD/CallPathsGroups.h>
#include <LibBDD/Visitors/Visitor.h>
#include <LibCore/Solver.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

#include <unordered_map>
#include <set>
#include <iostream>
#include <optional>

namespace LibBDD {

using LibCore::constraint_from_expr;
using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_struct_t;
using LibCore::expr_to_string;
using LibCore::is_bool;
using LibCore::simplify;
using LibCore::solver_toolbox;

namespace {
const std::unordered_set<std::string> ignored_functions{
    "start_time",
    "current_time",
    "loop_invariant_consume",
    "loop_invariant_produce",
    "packet_receive",
    "packet_state_total_length",
    "packet_get_unread_length",
};

constexpr const char *const init_to_process_trigger_function = "packet_receive";

const std::unordered_set<std::string> symbols_in_skippable_conditions{
    "received_a_packet",
    "loop_termination",
};

const std::unordered_set<std::string> routing_functions{
    "packet_send",
    "packet_free",
    "packet_broadcast",
};

const std::unordered_map<std::string, std::unordered_set<std::string>> symbols_from_call{
    {"current_time", {"next_time"}},
    {"packet_receive", {"DEVICE", "pkt_len"}},
    {"map_allocate", {"map_allocation_succeeded"}},
    {"vector_allocate", {"vector_alloc_success"}},
    {"dchain_allocate", {"is_dchain_allocated"}},
    {"cms_allocate", {"cms_allocation_succeeded"}},
    {"tb_allocate", {"tb_allocation_succeeded"}},
    {"lpm_allocate", {"lpm_alloc_success"}},
    {"rte_lcore_count", {"lcores"}},
    {"rte_ether_addr_hash", {"rte_ether_addr_hash"}},
    {"nf_set_rte_ipv4_udptcp_checksum", {"checksum"}},
    {"expire_items_single_map", {"number_of_freed_flows"}},
    {"expire_items_single_map_iteratively", {"number_of_freed_flows"}},
    {"map_get", {"map_has_this_key", "allocated_index"}},
    {"map_size", {"map_size"}},
    {"dchain_is_index_allocated", {"is_index_allocated"}},
    {"dchain_allocate_new_index", {"not_out_of_space", "new_index"}},
    {"vector_borrow", {"vector_data"}},
    {"vector_sample_lt", {"found_sample", "sample_index"}},
    {"cht_fill_cht", {"cht_fill_cht_successful"}},
    {"cht_find_preferred_available_backend", {"chosen_backend", "prefered_backend_found"}},
    {"cms_count_min", {"min_estimate"}},
    {"cms_periodic_cleanup", {"cleanup_success"}},
    {"hash_obj", {"hash"}},
    {"tb_is_tracing", {"is_tracing", "index_out"}},
    {"tb_trace", {"successfuly_tracing", "index_out"}},
    {"tb_update_and_check", {"pass"}},
    {"tb_expire", {"number_of_freed_flows"}},
    {"lpm_lookup", {"lpm_lookup_match", "lpm_lookup_result"}},
    {"lpm_update", {"lpm_update_elem_result"}},
};

typedef Symbols (*SymbolsExtractor)(const call_t &call, const Symbols &symbols);

Symbols packet_chunks_symbol_extractor(const call_t &call, const Symbols &symbols) {
  assert(call.function_name == "packet_borrow_next_chunk" && "Unexpected function");

  const extra_var_t &extra_var       = call.extra_vars.at("the_chunk");
  klee::ref<klee::Expr> packet_chunk = extra_var.second;

  symbol_t symbol = symbols.get("packet_chunks");
  symbol.expr     = packet_chunk;

  Symbols extracted_symbols;
  extracted_symbols.add(symbol);

  return extracted_symbols;
}

const std::unordered_map<std::string, SymbolsExtractor> special_symbols_extractors{
    {"packet_borrow_next_chunk", packet_chunks_symbol_extractor},
};

bool is_skip_function(const call_t &call) {
  return std::find(ignored_functions.begin(), ignored_functions.end(), call.function_name) != ignored_functions.end();
}

bool is_routing_function(const call_t &call) {
  return std::find(routing_functions.begin(), routing_functions.end(), call.function_name) != routing_functions.end();
}

bool is_skip_condition(klee::ref<klee::Expr> condition) {
  std::unordered_set<std::string> symbols = symbol_t::get_symbols_names(condition);

  for (const std::string &symbol : symbols) {
    auto base_symbol_comparator = [symbol](const std::string &s) { return symbol.find(s) != std::string::npos; };

    auto found_it = std::find_if(symbols_in_skippable_conditions.begin(), symbols_in_skippable_conditions.end(), base_symbol_comparator);

    if (found_it != symbols_in_skippable_conditions.end())
      return true;
  }

  return false;
}

Route *route_node_from_call(const call_t &call, bdd_node_id_t id, const klee::ConstraintManager &constraints, SymbolManager *symbol_manager) {
  assert(is_routing_function(call) && "Unexpected function");

  if (call.function_name == "packet_free") {
    return new Route(id, constraints, symbol_manager, RouteOp::Drop);
  }

  if (call.function_name == "packet_broadcast") {
    return new Route(id, constraints, symbol_manager, RouteOp::Broadcast);
  }

  assert(call.function_name == "packet_send" && "Unexpected function");

  klee::ref<klee::Expr> dst_device = call.args.at("dst_device").expr;
  assert(!dst_device.isNull() && "Null dst_device");

  return new Route(id, constraints, symbol_manager, RouteOp::Forward, dst_device);
}

call_t get_successful_call(const std::vector<call_path_t *> &call_paths) {
  assert(!call_paths.empty() && "No call paths");

  for (call_path_t *cp : call_paths) {
    assert(cp->calls.size() && "No calls");
    const call_t &call = cp->calls[0];

    if (call.ret.isNull()) {
      return call;
    }

    klee::ref<klee::Expr> zero    = solver_toolbox.exprBuilder->Constant(0, call.ret->getWidth());
    klee::ref<klee::Expr> eq_zero = solver_toolbox.exprBuilder->Eq(call.ret, zero);
    const bool is_ret_success     = solver_toolbox.is_expr_always_false(cp->constraints, eq_zero);

    if (is_ret_success) {
      return call;
    }
  }

  // No function with successful return.
  assert_or_panic(!call_paths[0]->calls.empty(), "No calls in first call path");
  return call_paths[0]->calls[0];
}

klee::ref<klee::Expr> simplify_constraint(klee::ref<klee::Expr> constraint) {
  assert(!constraint.isNull() && "Null constraint");

  klee::ref<klee::Expr> simplified = simplify(constraint);

  if (is_bool(simplified))
    return simplified;

  klee::Expr::Width width           = simplified->getWidth();
  klee::ref<klee::Expr> zero        = solver_toolbox.exprBuilder->Constant(0, width);
  klee::ref<klee::Expr> is_not_zero = solver_toolbox.exprBuilder->Ne(simplified, zero);

  return is_not_zero;
}

klee::ref<klee::Expr> negate_and_simplify_constraint(klee::ref<klee::Expr> constraint) {
  assert(!constraint.isNull() && "Null constraint");

  klee::ref<klee::Expr> simplified = simplify(constraint);

  if (is_bool(simplified))
    return solver_toolbox.exprBuilder->Not(simplified);

  klee::Expr::Width width       = simplified->getWidth();
  klee::ref<klee::Expr> zero    = solver_toolbox.exprBuilder->Constant(0, width);
  klee::ref<klee::Expr> is_zero = solver_toolbox.exprBuilder->Eq(simplified, zero);

  return is_zero;
}

std::optional<symbol_t> get_generated_symbol(const call_t &call, const Symbols &symbols, const std::string &base) {
  std::vector<symbol_t> filtered = symbols.filter_by_base(base).get();

  if (symbol_t::is_symbol(call.ret)) {
    symbol_t symbol(call.ret);
    if (base == symbol.base) {
      return symbol;
    }
  }

  for (const auto &[arg_name, arg] : call.args) {
    if (symbol_t::is_symbol(arg.out)) {
      symbol_t symbol(arg.out);
      if (base == symbol.base) {
        return symbol;
      }
    }
  }

  for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
    if (symbol_t::is_symbol(extra_var.second)) {
      symbol_t symbol(extra_var.second);
      if (base == symbol.base) {
        return symbol;
      }
    }
  }

  for (const auto &[arg_name, arg] : call.args) {
    if (symbol_t::is_symbol(arg.expr)) {
      symbol_t symbol(arg.expr);
      if (base == symbol.base) {
        return symbol;
      }
    }
  }

  return {};
}

Symbols get_generated_symbols(const call_t &call, Symbols &symbols, std::unordered_map<std::string, size_t> &base_symbols_generated) {
  Symbols generated_symbols;

  auto extractor_it = special_symbols_extractors.find(call.function_name);
  if (extractor_it != special_symbols_extractors.end()) {
    return extractor_it->second(call, symbols);
  }

  auto base_symbols_it = symbols_from_call.find(call.function_name);
  if (base_symbols_it == symbols_from_call.end()) {
    return generated_symbols;
  }

  for (const std::string &base : base_symbols_it->second) {
    std::optional<symbol_t> symbol = get_generated_symbol(call, symbols, base);
    if (symbol.has_value()) {
      generated_symbols.add(symbol.value());
    }
  }

  return generated_symbols;
}

void pop_call_paths(call_paths_view_t &call_paths_view) {
  for (call_path_t *cp : call_paths_view.data) {
    assert(cp->calls.size() && "No calls");
    cp->calls.erase(cp->calls.begin());
  }
}

BDDNode *bdd_from_call_paths(call_paths_view_t call_paths_view, SymbolManager *symbol_manager, BDDNodeManager &node_manager,
                             std::vector<Call *> &init, bdd_node_id_t &id, bool in_init_mode = true,
                             klee::ConstraintManager constraints                            = klee::ConstraintManager(),
                             std::unordered_map<std::string, size_t> base_symbols_generated = std::unordered_map<std::string, size_t>()) {
  BDDNode *root = nullptr;
  BDDNode *leaf = nullptr;

  assert(!call_paths_view.data.empty() && "No call paths");
  if (call_paths_view.data[0]->calls.size() == 0) {
    return root;
  }

  Symbols symbols = call_paths_view.get_symbols();

  while (!call_paths_view.data.empty()) {
    CallPathsGroup group(call_paths_view, in_init_mode);

    const call_paths_view_t &on_true  = group.get_on_true();
    const call_paths_view_t &on_false = group.get_on_false();

    assert(!on_true.data.empty() && "No call paths");
    if (on_true.data[0]->calls.size() == 0) {
      break;
    }

    if (on_false.data.empty()) {
      call_paths_view = on_true;

      const call_t call               = get_successful_call(call_paths_view.data);
      const Symbols generated_symbols = get_generated_symbols(call, symbols, base_symbols_generated);

      std::cerr << "\n";
      std::cerr << "==================================\n";
      std::cerr << "Call: " << call << "\n";
      std::cerr << "Call paths:\n";
      for (const call_path_t *cp : call_paths_view.data)
        std::cerr << "  " << cp->file_name << "\n";
      std::cerr << "Generated symbols (" << generated_symbols.size() << "):\n";
      for (const symbol_t &symbol : generated_symbols.get())
        std::cerr << "  " << expr_to_string(symbol.expr, true) << "\n";
      std::cerr << "Constraints (" << constraints.size() << "):\n";
      for (const klee::ref<klee::Expr> &constraint : constraints)
        std::cerr << "  " << expr_to_string(constraint, true) << "\n";
      std::cerr << "==================================\n";

      if (call.function_name == init_to_process_trigger_function) {
        in_init_mode = false;
        for (klee::ref<klee::Expr> common_constraint : group.get_common_constraints()) {
          constraints.addConstraint(common_constraint);
        }
      }

      if (is_skip_function(call)) {
        pop_call_paths(call_paths_view);
        continue;
      }

      if (in_init_mode) {
        Call *node = new Call(id, constraints, symbol_manager, call, generated_symbols);
        node_manager.add_node(node);
        id++;

        if (!init.empty()) {
          init.back()->set_next(node);
          node->set_prev(init.back());
        }

        init.push_back(node);
      } else {
        BDDNode *node;

        if (is_routing_function(call)) {
          node = route_node_from_call(call, id, constraints, symbol_manager);
        } else {
          node = new Call(id, constraints, symbol_manager, call, generated_symbols);
        }

        node_manager.add_node(node);
        id++;

        if (root == nullptr) {
          root = node;
          leaf = node;
        } else {
          leaf->set_next(node);
          node->set_prev(leaf);
          leaf = node;
        }
      }

      pop_call_paths(call_paths_view);
    } else {
      klee::ref<klee::Expr> discriminating_constraint = group.get_discriminating_constraint();

      klee::ref<klee::Expr> condition     = simplify_constraint(discriminating_constraint);
      klee::ref<klee::Expr> not_condition = negate_and_simplify_constraint(discriminating_constraint);

      klee::ConstraintManager on_true_constraints  = constraints;
      klee::ConstraintManager on_false_constraints = constraints;

      on_true_constraints.addConstraint(condition);
      on_false_constraints.addConstraint(not_condition);

      std::cerr << "\n";
      std::cerr << "==================================\n";
      std::cerr << "Condition: " << expr_to_string(condition, true) << "\n";
      std::cerr << "On true call paths:\n";
      for (const call_path_t *cp : on_true.data)
        std::cerr << "  " << cp->file_name << "\n";
      std::cerr << "On False call paths:\n";
      for (const call_path_t *cp : on_false.data)
        std::cerr << "  " << cp->file_name << "\n";
      std::cerr << "==================================\n";

      if (is_skip_condition(condition)) {
        // Assumes the right path is the one with the most call paths (or the on
        // true path, somewhat arbitrarily...)
        if (on_true.data.size() >= on_false.data.size()) {
          call_paths_view = on_true;
        } else {
          call_paths_view = on_false;
        }
        continue;
      }

      Branch *node = new Branch(id, constraints, symbol_manager, condition);
      id++;
      node_manager.add_node(node);

      BDDNode *on_true_root =
          bdd_from_call_paths(on_true, symbol_manager, node_manager, init, id, in_init_mode, on_true_constraints, base_symbols_generated);
      BDDNode *on_false_root =
          bdd_from_call_paths(on_false, symbol_manager, node_manager, init, id, in_init_mode, on_false_constraints, base_symbols_generated);

      if (!on_true_root || !on_false_root) {
        std::stringstream ss;
        const call_paths_view_t &failed_group = on_true_root == nullptr ? on_true : on_false;
        ss << "Failed to build nodes for the call paths:\n";
        for (const call_path_t *cp : failed_group.data) {
          ss << "  " << cp->file_name << "\n";
        }
        panic("%s", ss.str().c_str());
      }

      node->set_on_true(on_true_root);
      node->set_on_false(on_false_root);

      on_true_root->set_prev(node);
      on_false_root->set_prev(node);

      if (root == nullptr)
        return node;

      leaf->set_next(node);
      node->set_prev(leaf);

      return root;
    }
  }

  return root;
}

Branch *create_new_branch(BDD *bdd, const BDDNode *current, klee::ref<klee::Expr> condition) {
  bdd_node_id_t &id                   = bdd->get_mutable_id();
  BDDNodeManager &manager             = bdd->get_mutable_manager();
  klee::ConstraintManager constraints = current->get_constraints();
  Branch *new_branch                  = new Branch(id++, constraints, bdd->get_mutable_symbol_manager(), condition);
  manager.add_node(new_branch);
  return new_branch;
}

Call *create_new_call(BDD *bdd, const BDDNode *current, const call_t &call, const Symbols &generated_symbols) {
  bdd_node_id_t &id                   = bdd->get_mutable_id();
  BDDNodeManager &manager             = bdd->get_mutable_manager();
  klee::ConstraintManager constraints = current->get_constraints();
  Call *new_call                      = new Call(id++, constraints, bdd->get_mutable_symbol_manager(), call, generated_symbols);
  manager.add_node(new_call);
  return new_call;
}

struct next_t {
  struct obj_op_t {
    addr_t obj;
    const Call *call_node;

    bool operator==(const obj_op_t &other) const { return obj == other.obj; }
  };

  struct obj_op_hash_t {
    size_t operator()(const obj_op_t &obj_op) const { return obj_op.obj; }
  };

  typedef std::unordered_set<obj_op_t, obj_op_hash_t> objs_ops_t;

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

next_t get_next_maps_and_vectors(const BDDNode *root, klee::ref<klee::Expr> index) {
  next_t candidates;

  root->visit_nodes([&candidates, index](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name == "map_put") {
      klee::ref<klee::Expr> map   = call.args.at("map").expr;
      klee::ref<klee::Expr> value = call.args.at("value").expr;

      const addr_t map_addr = expr_addr_to_obj_addr(map);
      bool same_index       = solver_toolbox.are_exprs_always_equal(index, value);

      if (same_index) {
        candidates.maps.insert({map_addr, call_node});
      }
    } else if (call.function_name == "vector_borrow") {
      klee::ref<klee::Expr> vector = call.args.at("vector").expr;
      klee::ref<klee::Expr> value  = call.args.at("index").expr;

      const addr_t vector_addr = expr_addr_to_obj_addr(vector);
      bool same_index          = solver_toolbox.are_exprs_always_equal(index, value);

      if (same_index) {
        candidates.vectors.insert({vector_addr, call_node});
      }
    }

    return BDDNodeVisitAction::Continue;
  });

  return candidates;
}

next_t get_allowed_coalescing_objs(std::vector<const Call *> index_allocators, addr_t obj) {
  next_t candidates;

  for (const Call *allocator : index_allocators) {
    const call_t &allocator_call = allocator->get_call();

    klee::ref<klee::Expr> allocated_index = allocator_call.args.at("index_out").out;
    klee::ref<klee::Expr> dchain          = allocator_call.args.at("chain").expr;
    const addr_t dchain_addr              = expr_addr_to_obj_addr(dchain);

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

addr_t get_vector_obj_storing_map_key(const BDD *bdd, addr_t map) {
  const std::vector<const Call *> vector_borrows = bdd->get_root()->get_future_functions({"vector_borrow"});

  for (const Call *vector_borrow : vector_borrows) {
    const call_t &vb = vector_borrow->get_call();

    klee::ref<klee::Expr> vector_expr     = vb.args.at("vector").expr;
    klee::ref<klee::Expr> value_addr_expr = vb.args.at("val_out").out;

    const addr_t vector_obj = expr_addr_to_obj_addr(vector_expr);
    const addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    const std::vector<const Call *> map_puts = vector_borrow->get_future_functions({"map_put"});

    bool is_vector_key = false;
    for (const Call *map_put : map_puts) {
      const call_t &mp = map_put->get_call();

      klee::ref<klee::Expr> map_expr      = mp.args.at("map").expr;
      klee::ref<klee::Expr> key_addr_expr = mp.args.at("key").expr;

      const addr_t map_obj  = expr_addr_to_obj_addr(map_expr);
      const addr_t key_addr = expr_addr_to_obj_addr(key_addr_expr);

      if (map_obj != map) {
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

} // namespace

void BDD::visit(BDDVisitor &visitor) const { visitor.visit(this); }

const BDDNode *BDD::get_node_by_id(bdd_node_id_t node_id) const { return root ? root->get_node_by_id(node_id) : nullptr; }

BDDNode *BDD::get_mutable_node_by_id(bdd_node_id_t node_id) { return root ? root->get_mutable_node_by_id(node_id) : nullptr; }

int BDD::get_node_depth(bdd_node_id_t node_id) const {
  int depth = -1;

  if (!root) {
    return depth;
  }

  struct depth_tracker_t : cookie_t {
    int depth;

    depth_tracker_t() : depth(0) {}
    depth_tracker_t(int _depth) : depth(_depth) {}

    std::unique_ptr<cookie_t> clone() const override { return std::make_unique<depth_tracker_t>(depth); }
  };

  root->visit_nodes(
      [node_id, &depth](const BDDNode *node, cookie_t *cookie) {
        depth_tracker_t *depth_tracker = dynamic_cast<depth_tracker_t *>(cookie);

        if (node->get_id() == node_id) {
          depth = depth_tracker->depth;
          return BDDNodeVisitAction::Stop;
        }

        depth_tracker->depth++;
        return BDDNodeVisitAction::Continue;
      },
      std::make_unique<depth_tracker_t>());

  return depth;
}

Symbols BDD::get_generated_symbols(const BDDNode *node) const {
  Symbols symbols;
  symbols.add(device);
  symbols.add(packet_len);
  symbols.add(time);

  while (node) {
    if (node->get_type() == BDDNodeType::Call) {
      const Call *call_node       = dynamic_cast<const Call *>(node);
      const Symbols &more_symbols = call_node->get_local_symbols();
      symbols.add(more_symbols);
    }

    node = node->get_prev();
  }

  return symbols;
}

BDD::BDD(SymbolManager *_symbol_manager) : id(0), root(nullptr), symbol_manager(_symbol_manager) {
  assert(symbol_manager && "Symbol manager cannot be null");
}

BDD::BDD(const call_paths_view_t &call_paths_view) : id(0), symbol_manager(call_paths_view.manager) {
  root = bdd_from_call_paths(call_paths_view, symbol_manager, manager, init, id);

  packet_len = symbol_manager->get_symbol("pkt_len");
  time       = symbol_manager->get_symbol("next_time");

  // Some NFs don't care about the device, so it doesn't show up on the callpaths.
  if (symbol_manager->has_symbol("DEVICE")) {
    device = call_paths_view.manager->get_symbol("DEVICE");
  } else {
    device = call_paths_view.manager->create_symbol("DEVICE", 32);
  }
}

BDD::BDD(const std::filesystem::path &fpath, SymbolManager *_symbol_manager) : id(0), symbol_manager(_symbol_manager) { deserialize(fpath); }

BDD::BDD(const BDD &other)
    : id(other.id), device(other.device), packet_len(other.packet_len), time(other.time), symbol_manager(other.symbol_manager) {
  for (const Call *init_node : other.init) {
    Call *cloned = dynamic_cast<Call *>(init_node->clone(manager));
    if (!init.empty()) {
      init.back()->set_next(cloned);
      cloned->set_prev(init.back());
    }
    init.push_back(cloned);
  }
  root = other.root->clone(manager, true);
}

BDD::BDD(BDD &&other)
    : id(other.id), device(std::move(other.device)), packet_len(std::move(other.packet_len)), time(std::move(other.time)),
      init(std::move(other.init)), root(other.root), manager(std::move(other.manager)), symbol_manager(std::move(other.symbol_manager)) {
  other.root = nullptr;
}

BDD &BDD::operator=(const BDD &other) {
  if (this == &other)
    return *this;
  id         = other.id;
  device     = other.device;
  packet_len = other.packet_len;
  time       = other.time;

  for (const Call *init_node : other.init) {
    Call *cloned = dynamic_cast<Call *>(init_node->clone(manager));
    if (!init.empty()) {
      init.back()->set_next(cloned);
      cloned->set_prev(init.back());
    }
    init.push_back(cloned);
  }

  root           = other.root->clone(manager, true);
  symbol_manager = other.symbol_manager;
  return *this;
}

void BDD::delete_init_node(bdd_node_id_t target_id) {
  auto it = init.begin();
  while (it != init.end()) {
    if ((*it)->get_id() == target_id) {
      Call *node    = *it;
      BDDNode *prev = node->get_mutable_prev();
      BDDNode *next = node->get_mutable_next();

      if (prev) {
        assert(prev->get_type() == BDDNodeType::Call && "Unexpected node type");
        prev->set_next(next);
      }

      if (next) {
        assert(next->get_type() == BDDNodeType::Call && "Unexpected node type");
        next->set_prev(prev);
      }

      it = init.erase(it);
      manager.free_node(node);

      return;
    }

    it++;
  }
}

BDDNode *BDD::delete_non_branch(bdd_node_id_t target_id) { return delete_non_branch(get_mutable_node_by_id(target_id), manager); }

BDDNode *BDD::delete_branch(bdd_node_id_t target_id, bool direction_to_keep) {
  return delete_branch(get_mutable_node_by_id(target_id), direction_to_keep, manager);
}

BDDNode *BDD::add_cloned_non_branches(bdd_node_id_t target_id, const std::vector<const BDDNode *> &new_nodes) {
  const BDDNode *current = get_node_by_id(target_id);
  assert(current && "BDDNode not found");

  BDDNode *new_current = nullptr;

  if (new_nodes.empty()) {
    return new_current;
  }

  const BDDNode *prev = current->get_prev();
  assert(prev && "No previous node");

  bdd_node_id_t anchor_id = prev->get_id();
  BDDNode *anchor         = get_mutable_node_by_id(anchor_id);
  BDDNode *anchor_next    = get_mutable_node_by_id(current->get_id());

  bool set_new_current = false;

  for (const BDDNode *new_node : new_nodes) {
    assert(new_node->get_type() != BDDNodeType::Branch && "Unexpected branch node");

    BDDNode *clone = new_node->clone(manager, false);
    clone->recursive_update_ids(id);

    if (!set_new_current) {
      new_current     = clone;
      set_new_current = true;
    }

    switch (anchor->get_type()) {
    case BDDNodeType::Call:
    case BDDNodeType::Route: {
      anchor->set_next(clone);
    } break;
    case BDDNodeType::Branch: {
      Branch *branch = dynamic_cast<Branch *>(anchor);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();

      assert_or_panic((on_true == anchor_next || on_false == anchor_next), "No connection found");

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

Branch *BDD::add_cloned_branch(bdd_node_id_t target_id, klee::ref<klee::Expr> condition) {
  const BDDNode *current = get_node_by_id(target_id);
  assert(current && "BDDNode not found");

  const BDDNode *prev = current->get_prev();
  assert(prev && "No previous node");

  bdd_node_id_t anchor_id = prev->get_id();
  BDDNode *anchor         = get_mutable_node_by_id(anchor_id);
  BDDNode *anchor_next    = get_mutable_node_by_id(current->get_id());

  klee::ref<klee::Expr> constraint = constraint_from_expr(condition);

  BDDNode *on_true_cond  = anchor_next;
  BDDNode *on_false_cond = anchor_next->clone(manager, true);
  on_false_cond->recursive_update_ids(id);

  on_true_cond->recursive_add_constraint(constraint);
  on_false_cond->recursive_add_constraint(solver_toolbox.exprBuilder->Not(constraint));

  Branch *new_branch = create_new_branch(this, current, condition);

  new_branch->set_on_true(on_true_cond);
  new_branch->set_on_false(on_false_cond);

  on_true_cond->set_prev(new_branch);
  on_false_cond->set_prev(new_branch);

  switch (anchor->get_type()) {
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    anchor->set_next(new_branch);
  } break;
  case BDDNodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const BDDNode *on_true  = branch->get_on_true();
    const BDDNode *on_false = branch->get_on_false();

    assert_or_panic((on_true == anchor_next || on_false == anchor_next), "No connection found");

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

Call *BDD::add_new_symbol_generator_function(bdd_node_id_t target_id, const std::string &fn_name, const Symbols &symbols) {
  const BDDNode *current = get_node_by_id(target_id);
  assert(current && "BDDNode not found");

  const BDDNode *prev = current->get_prev();
  assert(prev && "No previous node");

  const call_t call{
      .function_name = fn_name,
      .extra_vars    = {},
      .args          = {},
      .ret           = {},
  };

  Call *new_node = create_new_call(this, current, call, symbols);

  bdd_node_id_t anchor_id = prev->get_id();
  BDDNode *anchor         = get_mutable_node_by_id(anchor_id);
  BDDNode *anchor_next    = get_mutable_node_by_id(current->get_id());

  switch (anchor->get_type()) {
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    anchor->set_next(new_node);
  } break;
  case BDDNodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const BDDNode *on_true  = branch->get_on_true();
    const BDDNode *on_false = branch->get_on_false();

    assert_or_panic((on_true == anchor_next || on_false == anchor_next), "No connection found");

    if (on_true == anchor_next) {
      branch->set_on_true(new_node);
    } else {
      branch->set_on_false(new_node);
    }

  } break;
  }

  new_node->set_prev(anchor);
  new_node->set_next(anchor_next);
  anchor_next->set_prev(new_node);

  return new_node;
}

bool BDD::get_map_coalescing_objs(addr_t obj, map_coalescing_objs_t &data) const {
  const std::vector<const Call *> index_allocators = root->get_future_functions({"dchain_allocate_new_index"});

  if (index_allocators.empty()) {
    return false;
  }

  const next_t candidates = get_allowed_coalescing_objs(index_allocators, obj);

  if (candidates.size() == 0) {
    return false;
  }

  assert(candidates.maps.size() == 1 && "Expecting a single map");
  assert(candidates.dchains.size() == 1 && "Expecting a single dchain");

  data.map    = candidates.maps.begin()->obj;
  data.dchain = candidates.dchains.begin()->obj;

  for (const auto &vector : candidates.vectors) {
    data.vectors.insert(vector.obj);
  }

  return true;
}

bool BDD::get_map_coalescing_objs_from_map_op(const Call *map_op, map_coalescing_objs_t &map_objs) const {
  const call_t &call = map_op->get_call();

  addr_t obj;
  if (call.args.find("map") != call.args.end()) {
    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
  } else if (call.args.find("map_out") != call.args.end()) {
    obj = expr_addr_to_obj_addr(call.args.at("map_out").out);
  } else {
    panic("No map argument");
  }

  return get_map_coalescing_objs(obj, map_objs);
}

bool BDD::get_map_coalescing_objs_from_dchain_op(const Call *dchain_op, map_coalescing_objs_t &map_objs) const {
  const call_t &call = dchain_op->get_call();

  addr_t obj;
  if (call.args.find("chain") != call.args.end()) {
    obj = expr_addr_to_obj_addr(call.args.at("chain").expr);
  } else if (call.args.find("chain_out") != call.args.end()) {
    obj = expr_addr_to_obj_addr(call.args.at("chain_out").out);
  } else {
    panic("No chain argument");
  }

  return get_map_coalescing_objs(obj, map_objs);
}

bool BDD::is_index_alloc_on_unsuccessful_map_get(const Call *dchain_allocate_new_index) const {
  const call_t &call = dchain_allocate_new_index->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return false;
  }

  const BDDNode *map_get = dchain_allocate_new_index;
  while (map_get) {
    if (map_get->get_type() != BDDNodeType::Call) {
      map_get = map_get->get_prev();
      continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(map_get);
    const call_t &mg_call = call_node->get_call();

    if (mg_call.function_name != "map_get") {
      map_get = map_get->get_prev();
      continue;
    }

    klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
    if (expr_addr_to_obj_addr(obj) != map_objs.map) {
      map_get = map_get->get_prev();
      continue;
    }

    break;
  }

  if (!map_get) {
    return false;
  }

  symbol_t map_has_this_key           = dynamic_cast<const Call *>(map_get)->get_local_symbol("map_has_this_key");
  klee::ConstraintManager constraints = dchain_allocate_new_index->get_constraints();

  klee::ref<klee::Expr> found_key =
      solver_toolbox.exprBuilder->Ne(map_has_this_key.expr, solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  return solver_toolbox.is_expr_always_false(constraints, found_key);
}

bool BDD::is_map_update_with_dchain(const Call *dchain_allocate_new_index, std::vector<const Call *> &map_puts) const {
  const call_t &call = dchain_allocate_new_index->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return false;
  }

  const branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();

  if (!index_alloc_check.branch) {
    return false;
  }

  klee::ref<klee::Expr> condition           = index_alloc_check.branch->get_condition();
  std::vector<const Call *> future_map_puts = dchain_allocate_new_index->get_future_functions({"map_put"});

  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> map_expr = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> mp_key   = mp_call.args.at("key").in;
    klee::ref<klee::Expr> mp_value = mp_call.args.at("value").expr;

    const addr_t map = expr_addr_to_obj_addr(map_expr);

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

    if ((index_alloc_check.direction && !solver_toolbox.is_expr_always_true(constraints, condition)) ||
        (!index_alloc_check.direction && !solver_toolbox.is_expr_always_false(constraints, condition))) {
      return false;
    }

    map_puts.push_back(map_put);
  }

  if (map_puts.empty()) {
    return false;
  }

  return true;
}

bool BDD::is_fwd_pattern_depending_on_lpm(const BDDNode *node, std::vector<const BDDNode *> &fwd_logic) const {
  bool pattern_found = false;
  node->visit_nodes([&fwd_logic, &pattern_found](const BDDNode *future_node) {
    switch (future_node->get_type()) {
    case BDDNodeType::Call: {
      pattern_found = false;
    } break;
    case BDDNodeType::Branch: {
      const Branch *branch                                = dynamic_cast<const Branch *>(future_node);
      const std::unordered_set<std::string> &used_symbols = branch->get_used_symbols();
      if (used_symbols.size() == 1 && (*used_symbols.begin() == "lpm_lookup_match" || *used_symbols.begin() == "lpm_lookup_result")) {
        pattern_found = true;
        fwd_logic.push_back(future_node);
      } else {
        pattern_found = false;
      }
    } break;
    case BDDNodeType::Route: {
      if (pattern_found) {
        fwd_logic.push_back(future_node);
      }
    } break;
    }

    if (pattern_found) {
      return BDDNodeVisitAction::Continue;
    } else {
      return BDDNodeVisitAction::Stop;
    }
  });

  return pattern_found;
}

std::vector<u16> BDD::get_devices() const {
  std::vector<u16> devices;

  if (!root) {
    return devices;
  }

  const klee::ConstraintManager &base_constraints = root->get_constraints();
  for (u16 device_value = 0; device_value < UINT16_MAX; device_value++) {
    bool valid_device_value = solver_toolbox.is_expr_always_false(
        base_constraints, solver_toolbox.exprBuilder->Eq(device.expr, solver_toolbox.exprBuilder->Constant(device_value, device.expr->getWidth())));

    if (valid_device_value) {
      break;
    } else {
      devices.push_back(device_value);
    }
  }

  return devices;
}

BDD::inspection_report_t BDD::inspect() const {
  if (!root) {
    return {InspectionStatus::MissingRootNode, "Missing root node"};
  }

  for (size_t i = 0; i < init.size(); i++) {
    const Call *init_node = init[i];

    if (!init_node) {
      return {InspectionStatus::HasNullNode, "Has null node (in init group)"};
    }

    if (i < init.size() - 1) {
      if (!init_node->get_next()) {
        return {InspectionStatus::DanglingInitNode, "Init node " + std::to_string(init_node->get_id()) + " is not connected"};
      }

      if (init_node->get_next()->get_prev() != init_node) {
        return {InspectionStatus::BrokenLink, "Init node " + std::to_string(init_node->get_id()) + " has invalid link"};
      }
    }
  }

  inspection_report_t report                                 = {InspectionStatus::Ok, "Ok"};
  const std::unordered_set<std::string> symbols_always_known = {"DEVICE", "pkt_len", "next_time", "packet_chunks"};
  std::unordered_set<const BDDNode *> visited_nodes;

  root->visit_nodes([&](const BDDNode *node) {
    if (!manager.has_node(node)) {
      report = {InspectionStatus::UnmanagedNode, "Unmanaged BDDNode " + std::to_string(node->get_id())};
      return BDDNodeVisitAction::Stop;
    }

    if (visited_nodes.find(node) != visited_nodes.end()) {
      report = {InspectionStatus::HasCycle, "Has cycle in the BDD"};
      return BDDNodeVisitAction::Stop;
    }

    visited_nodes.insert(node);

    if (!node) {
      report = {InspectionStatus::HasNullNode, "Has null node"};
      return BDDNodeVisitAction::Stop;
    }

    const Symbols used_symbols      = node->get_used_symbols();
    const Symbols available_symbols = node->get_prev_symbols();

    for (const symbol_t &used_symbol : used_symbols.get()) {
      if (symbols_always_known.find(used_symbol.name) != symbols_always_known.end()) {
        continue;
      }

      if (!available_symbols.has(used_symbol.name)) {
        report = {InspectionStatus::MissingSymbol, "BDDNode " + std::to_string(node->get_id()) + " uses unavailable symbol " + used_symbol.name};
        return BDDNodeVisitAction::Stop;
      }
    }

    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch    = dynamic_cast<const Branch *>(node);
      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();

      if (!on_true) {
        report = {InspectionStatus::BranchWithoutChildren, "Branch node " + std::to_string(node->get_id()) + " without on true side"};
        return BDDNodeVisitAction::Stop;
      }

      if (!on_false) {
        report = {InspectionStatus::BranchWithoutChildren, "Branch node " + std::to_string(node->get_id()) + " without on false side"};
        return BDDNodeVisitAction::Stop;
      }

      if (on_true->get_prev() != node) {
        report = {InspectionStatus::BrokenLink, "Branch node " + std::to_string(node->get_id()) + " on true side has invalid link"};
        return BDDNodeVisitAction::Stop;
      }

      if (on_false->get_prev() != node) {
        report = {InspectionStatus::BrokenLink, "Branch node " + std::to_string(node->get_id()) + " on false side has invalid link"};
        return BDDNodeVisitAction::Stop;
      }
    } break;
    case BDDNodeType::Call: {
      if (node->get_next() && node->get_next()->get_prev() != node) {
        report = {InspectionStatus::BrokenLink, "Call node " + std::to_string(node->get_id()) + " has invalid link"};
        return BDDNodeVisitAction::Stop;
      }
    } break;

    case BDDNodeType::Route: {
      if (node->get_next() && node->get_next()->get_prev() != node) {
        report = {InspectionStatus::BrokenLink, "Route node " + std::to_string(node->get_id()) + " has invalid link"};
        return BDDNodeVisitAction::Stop;
      }
    } break;
    }

    return BDDNodeVisitAction::Continue;
  });

  return report;
}

void BDD::delete_vector_key_operations(addr_t map) {
  std::unordered_set<const BDDNode *> candidates;
  Symbols key_symbols;

  const addr_t vector_key = get_vector_obj_storing_map_key(this, map);

  root->visit_nodes([vector_key, &candidates, &key_symbols](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return BDDNodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
    const addr_t obj               = expr_addr_to_obj_addr(obj_expr);

    if (obj != vector_key) {
      return BDDNodeVisitAction::Continue;
    }

    if (call.function_name == "vector_borrow") {
      key_symbols.add(call_node->get_local_symbols());
    }

    candidates.insert(node);

    return BDDNodeVisitAction::Continue;
  });

  bool used = false;
  root->visit_nodes([&used, &candidates, &key_symbols](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    if (candidates.find(node) != candidates.end()) {
      return BDDNodeVisitAction::Continue;
    }

    const Symbols symbols = node->get_used_symbols();
    if (!symbols.intersect(key_symbols).empty()) {
      used = true;
      return BDDNodeVisitAction::Stop;
    }

    return BDDNodeVisitAction::Continue;
  });

  if (used) {
    return;
  }

  for (const BDDNode *node : candidates) {
    delete_non_branch(node->get_id());
  }

  for (const Call *call : init) {
    if (call->get_call().function_name != "vector_allocate") {
      continue;
    }

    klee::ref<klee::Expr> obj_expr = call->get_call().args.at("vector_out").out;
    const addr_t obj               = expr_addr_to_obj_addr(obj_expr);

    if (obj != vector_key) {
      continue;
    }

    delete_init_node(call->get_id());
    break;
  }

  // Just to double check that we didn't break anything...
  const BDD::inspection_report_t report = inspect();
  if (report.status != BDD::InspectionStatus::Ok) {
    panic("BDD inspection failed: %s", report.message.c_str());
  }
}

BDDNode *BDD::delete_non_branch(BDDNode *anchor_next, BDDNodeManager &manager) {
  assert(anchor_next && "BDDNode not found");
  assert(anchor_next->get_type() != BDDNodeType::Branch && "Unexpected branch node");

  BDDNode *anchor = anchor_next->get_mutable_prev();
  assert(anchor && "No previous node");

  BDDNode *new_current = anchor_next->get_mutable_next();

  switch (anchor->get_type()) {
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    anchor->set_next(new_current);
  } break;
  case BDDNodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const BDDNode *on_true  = branch->get_on_true();
    const BDDNode *on_false = branch->get_on_false();

    assert_or_panic((on_true == anchor_next || on_false == anchor_next), "No connection");

    if (on_true == anchor_next) {
      branch->set_on_true(new_current);
    } else {
      branch->set_on_false(new_current);
    }

  } break;
  }

  if (new_current) {
    new_current->set_prev(anchor);
  }

  manager.free_node(anchor_next);

  return new_current;
}

BDDNode *BDD::delete_branch(BDDNode *target, bool direction_to_keep, BDDNodeManager &manager) {
  assert(target && "BDDNode not found");
  assert(target->get_type() == BDDNodeType::Branch && "Unexpected branch node");

  BDDNode *anchor = target->get_mutable_prev();
  assert(anchor && "No previous node");

  Branch *anchor_next = dynamic_cast<Branch *>(target);

  BDDNode *target_on_true  = anchor_next->get_mutable_on_true();
  BDDNode *target_on_false = anchor_next->get_mutable_on_false();

  BDDNode *new_current;

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
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    anchor->set_next(new_current);
  } break;
  case BDDNodeType::Branch: {
    Branch *branch = dynamic_cast<Branch *>(anchor);

    const BDDNode *on_true  = branch->get_on_true();
    const BDDNode *on_false = branch->get_on_false();

    assert_or_panic((on_true == anchor_next || on_false == anchor_next), "No connection");

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

} // namespace LibBDD