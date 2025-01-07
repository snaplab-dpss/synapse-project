#include <unordered_map>
#include <set>
#include <iostream>
#include <optional>

#include "bdd.h"
#include "call_paths_groups.h"
#include "visitor.h"
#include "nodes/branch.h"
#include "nodes/call.h"
#include "nodes/route.h"
#include "../util/exprs.h"
#include "../util/simplifier.h"
#include "../util/solver.h"

namespace synapse {
namespace {
const std::vector<std::string> ignored_functions{
    "start_time",
    "current_time",
    "loop_invariant_consume",
    "loop_invariant_produce",
    "packet_receive",
    "packet_state_total_length",
    "packet_get_unread_length",
    "vector_reset",
};

const std::vector<std::string> init_functions{
    "map_allocate", "vector_allocate", "dchain_allocate",
    "cms_allocate", "cht_fill_cht",    "tb_allocate",
};

const std::vector<std::string> symbols_in_skippable_conditions{
    "received_a_packet",        "loop_termination",        "map_allocation_succeeded",
    "vector_alloc_success",     "is_dchain_allocated",     "cht_fill_cht_successful",
    "cms_allocation_succeeded", "tb_allocation_succeeded",
};

const std::vector<std::string> rounting_functions{
    "packet_send",
    "packet_free",
    "packet_broadcast",
};

const std::unordered_map<std::string, std::unordered_set<std::string>> symbols_from_call{
    {
        "rte_lcore_count",
        {"lcores"},
    },
    {
        "rte_ether_addr_hash",
        {"rte_ether_addr_hash"},
    },
    {
        "nf_set_rte_ipv4_udptcp_checksum",
        {"checksum"},
    },
    {
        "current_time",
        {"next_time"},
    },
    {
        "packet_receive",
        {"DEVICE", "pkt_len"},
    },
    {
        "expire_items_single_map",
        {"number_of_freed_flows"},
    },
    {
        "expire_items_single_map_iteratively",
        {"number_of_freed_flows"},
    },
    {
        "map_get",
        {"map_has_this_key", "allocated_index"},
    },
    {
        "map_size",
        {"map_size"},
    },
    {
        "dchain_is_index_allocated",
        {"dchain_is_index_allocated"},
    },
    {
        "dchain_allocate_new_index",
        {"out_of_space", "new_index"},
    },
    {
        "vector_borrow",
        {"vector_data_reset"},
    },
    {
        "vector_sample_lt",
        {"found_sample", "sample_index"},
    },
    {
        "cht_find_preferred_available_backend",
        {"chosen_backend", "prefered_backend_found"},
    },
    {
        "cms_count_min",
        {"min_estimate"},
    },
    {
        "cms_periodic_cleanup",
        {"cleanup_success"},
    },
    {
        "hash_obj",
        {"hash"},
    },
    {
        "tb_is_tracing",
        {"is_tracing"},
    },
    {
        "tb_trace",
        {"index_out"},
    },
    {
        "tb_update_and_check",
        {"pass"},
    },
    {
        "tb_expire",
        {"number_of_freed_flows"},
    },
};

typedef symbols_t (*SymbolsExtractor)(const call_t &call, const SymbolManager *manager);

symbols_t packet_chunks_symbol_extractor(const call_t &call, const SymbolManager *manager) {
  SYNAPSE_ASSERT(call.function_name == "packet_borrow_next_chunk", "Unexpected function");

  const extra_var_t &extra_var = call.extra_vars.at("the_chunk");
  klee::ref<klee::Expr> packet_chunk = extra_var.second;

  symbol_t symbol = manager->get_symbol("packet_chunks");
  symbol.expr = packet_chunk;

  return {symbol};
}

const std::unordered_map<std::string, SymbolsExtractor> special_symbols_extractors{
    {
        "packet_borrow_next_chunk",
        packet_chunks_symbol_extractor,
    },
};

bool is_init_function(const call_t &call) {
  return std::find(init_functions.begin(), init_functions.end(), call.function_name) !=
         init_functions.end();
}

bool is_skip_function(const call_t &call) {
  return std::find(ignored_functions.begin(), ignored_functions.end(), call.function_name) !=
         ignored_functions.end();
}

bool is_routing_function(const call_t &call) {
  return std::find(rounting_functions.begin(), rounting_functions.end(), call.function_name) !=
         rounting_functions.end();
}

bool is_skip_condition(klee::ref<klee::Expr> condition) {
  std::unordered_set<std::string> symbols = symbol_t::get_symbols_names(condition);

  for (const std::string &symbol : symbols) {
    auto base_symbol_comparator = [symbol](const std::string &s) {
      return symbol.find(s) != std::string::npos;
    };

    auto found_it = std::find_if(symbols_in_skippable_conditions.begin(),
                                 symbols_in_skippable_conditions.end(), base_symbol_comparator);

    if (found_it != symbols_in_skippable_conditions.end())
      return true;
  }

  return false;
}

Route *route_node_from_call(const call_t &call, node_id_t id,
                            const klee::ConstraintManager &constraints,
                            SymbolManager *symbol_manager) {
  SYNAPSE_ASSERT(is_routing_function(call), "Unexpected function");

  if (call.function_name == "packet_free") {
    return new Route(id, constraints, symbol_manager, RouteOp::Drop);
  }

  if (call.function_name == "packet_broadcast") {
    return new Route(id, constraints, symbol_manager, RouteOp::Broadcast);
  }

  SYNAPSE_ASSERT(call.function_name == "packet_send", "Unexpected function");

  klee::ref<klee::Expr> dst_device = call.args.at("dst_device").expr;
  SYNAPSE_ASSERT(!dst_device.isNull(), "Null dst_device");

  int value = solver_toolbox.value_from_expr(dst_device);
  return new Route(id, constraints, symbol_manager, RouteOp::Forward, value);
}

call_t get_successful_call(const std::vector<call_path_t *> &call_paths) {
  SYNAPSE_ASSERT(!call_paths.empty(), "No call paths");

  for (call_path_t *cp : call_paths) {
    SYNAPSE_ASSERT(cp->calls.size(), "No calls");
    const call_t &call = cp->calls[0];

    if (call.ret.isNull())
      return call;

    klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, call.ret->getWidth());
    klee::ref<klee::Expr> eq_zero = solver_toolbox.exprBuilder->Eq(call.ret, zero);
    bool is_ret_success = solver_toolbox.is_expr_always_false(eq_zero);

    if (is_ret_success)
      return call;
  }

  // No function with successful return.
  SYNAPSE_ASSERT(!call_paths[0]->calls.empty(), "No calls in first call path");
  return call_paths[0]->calls[0];
}

klee::ref<klee::Expr> simplify_constraint(klee::ref<klee::Expr> constraint) {
  SYNAPSE_ASSERT(!constraint.isNull(), "Null constraint");

  klee::ref<klee::Expr> simplified = simplify(constraint);

  if (is_bool(simplified))
    return simplified;

  klee::Expr::Width width = simplified->getWidth();
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  klee::ref<klee::Expr> is_not_zero = solver_toolbox.exprBuilder->Ne(simplified, zero);

  return is_not_zero;
}

klee::ref<klee::Expr> negate_and_simplify_constraint(klee::ref<klee::Expr> constraint) {
  SYNAPSE_ASSERT(!constraint.isNull(), "Null constraint");

  klee::ref<klee::Expr> simplified = simplify(constraint);

  if (is_bool(simplified))
    return solver_toolbox.exprBuilder->Not(simplified);

  klee::Expr::Width width = simplified->getWidth();
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  klee::ref<klee::Expr> is_zero = solver_toolbox.exprBuilder->Eq(simplified, zero);

  return is_zero;
}

std::optional<symbol_t>
get_generated_symbol(const SymbolManager *manager, const std::string &base,
                     std::unordered_map<std::string, size_t> &base_symbols_generated) {
  symbols_t symbols = manager->get_symbols_with_base(base);

  auto symbol_cmp = [base](const symbol_t &a, const symbol_t &b) {
    SYNAPSE_ASSERT(a.base == b.base, "Different base symbols");

    size_t base_a_pos = a.name.find(base);
    size_t base_b_pos = b.name.find(base);

    std::string a_suffix = a.name.substr(base_a_pos + base.size());
    std::string b_suffix = b.name.substr(base_b_pos + base.size());

    if (a_suffix.size() == 0) {
      return true;
    }

    if (b_suffix.size() == 0) {
      return false;
    }

    SYNAPSE_ASSERT(a_suffix[0] == '_', "Invalid suffix");
    SYNAPSE_ASSERT(b_suffix[0] == '_', "Invalid suffix");

    int a_suffix_num = std::stoi(a_suffix.substr(1));
    int b_suffix_num = std::stoi(b_suffix.substr(1));

    return a_suffix_num < b_suffix_num;
  };

  std::vector<symbol_t> filtered;
  filtered.insert(filtered.end(), symbols.begin(), symbols.end());
  std::sort(filtered.begin(), filtered.end(), symbol_cmp);

  size_t total_base_symbols_generated = 0;
  if (base_symbols_generated.find(base) != base_symbols_generated.end()) {
    total_base_symbols_generated = base_symbols_generated[base];
  } else {
    base_symbols_generated[base] = 0;
  }

  if (total_base_symbols_generated >= filtered.size()) {
    // Not enough symbols for translation
    return std::nullopt;
  }

  base_symbols_generated[base]++;

  return filtered[total_base_symbols_generated];
}

symbols_t get_call_symbols(const call_t &call, const SymbolManager *manager,
                           std::unordered_map<std::string, size_t> &base_symbols_generated) {
  symbols_t symbols;

  auto extractor_it = special_symbols_extractors.find(call.function_name);
  if (extractor_it != special_symbols_extractors.end()) {
    return extractor_it->second(call, manager);
  }

  auto base_symbols_it = symbols_from_call.find(call.function_name);
  if (base_symbols_it == symbols_from_call.end()) {
    return symbols;
  }

  for (const std::string &base : base_symbols_it->second) {
    std::optional<symbol_t> symbol = get_generated_symbol(manager, base, base_symbols_generated);
    if (symbol.has_value()) {
      symbols.insert(symbol.value());
    }
  }

  return symbols;
}

void pop_call_paths(call_paths_view_t &call_paths_view) {
  for (call_path_t *cp : call_paths_view.data) {
    SYNAPSE_ASSERT(cp->calls.size(), "No calls");
    cp->calls.erase(cp->calls.begin());
  }
}

Node *bdd_from_call_paths(call_paths_view_t call_paths_view, SymbolManager *symbol_manager,
                          NodeManager &node_manager, std::vector<call_t> &init, node_id_t &id,
                          klee::ConstraintManager constraints = klee::ConstraintManager(),
                          std::unordered_map<std::string, size_t> base_symbols_generated =
                              std::unordered_map<std::string, size_t>()) {
  Node *root = nullptr;
  Node *leaf = nullptr;

  SYNAPSE_ASSERT(!call_paths_view.data.empty(), "No call paths");
  if (call_paths_view.data[0]->calls.size() == 0) {
    return root;
  }

  while (!call_paths_view.data.empty()) {
    CallPathsGroup group(call_paths_view);

    const call_paths_view_t &on_true = group.get_on_true();
    const call_paths_view_t &on_false = group.get_on_false();

    SYNAPSE_ASSERT(!on_true.data.empty(), "No call paths");
    if (on_true.data[0]->calls.size() == 0) {
      break;
    }

    if (on_true.data.size() == call_paths_view.data.size()) {
      SYNAPSE_ASSERT(on_false.data.empty(), "Unexpected call paths");

      call_t call = get_successful_call(call_paths_view.data);
      symbols_t call_symbols = get_call_symbols(call, symbol_manager, base_symbols_generated);

      std::cerr << "\n";
      std::cerr << "==================================\n";
      std::cerr << "Call: " << call << "\n";
      std::cerr << "Call paths:\n";
      for (const call_path_t *cp : call_paths_view.data)
        std::cerr << "  " << cp->file_name << "\n";
      std::cerr << "Generated symbols (" << call_symbols.size() << "):\n";
      for (const symbol_t &symbol : call_symbols)
        std::cerr << "  " << expr_to_string(symbol.expr, true) << "\n";
      std::cerr << "Constraints (" << constraints.size() << "):\n";
      for (const klee::ref<klee::Expr> &constraint : constraints)
        std::cerr << "  " << expr_to_string(constraint, true) << "\n";
      std::cerr << "==================================\n";

      if (is_init_function(call)) {
        init.push_back(call);
      } else if (!is_skip_function(call)) {
        Node *node;

        if (is_routing_function(call)) {
          node = route_node_from_call(call, id, constraints, symbol_manager);
        } else {
          node = new Call(id, constraints, symbol_manager, call, call_symbols);
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

      klee::ref<klee::Expr> condition = simplify_constraint(discriminating_constraint);
      klee::ref<klee::Expr> not_condition =
          negate_and_simplify_constraint(discriminating_constraint);

      klee::ConstraintManager on_true_constraints = constraints;
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

      Node *on_true_root = bdd_from_call_paths(on_true, symbol_manager, node_manager, init, id,
                                               on_true_constraints, base_symbols_generated);
      Node *on_false_root = bdd_from_call_paths(on_false, symbol_manager, node_manager, init, id,
                                                on_false_constraints, base_symbols_generated);

      SYNAPSE_ASSERT(on_true_root && on_false_root, "Invalid BDD");

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

Branch *create_new_branch(BDD *bdd, const Node *current, klee::ref<klee::Expr> condition) {
  node_id_t &id = bdd->get_mutable_id();
  NodeManager &manager = bdd->get_mutable_manager();
  klee::ConstraintManager constraints = current->get_constraints();
  Branch *new_branch = new Branch(id++, constraints, bdd->get_mutable_symbol_manager(), condition);
  manager.add_node(new_branch);
  return new_branch;
}
} // namespace

void BDD::visit(BDDVisitor &visitor) const { visitor.visit(this); }

const Node *BDD::get_node_by_id(node_id_t node_id) const {
  return root ? root->get_node_by_id(node_id) : nullptr;
}

Node *BDD::get_mutable_node_by_id(node_id_t node_id) {
  return root ? root->get_mutable_node_by_id(node_id) : nullptr;
}

int BDD::get_node_depth(node_id_t node_id) const {
  int depth = -1;

  if (!root) {
    return depth;
  }

  struct depth_tracker_t : cookie_t {
    int depth;

    depth_tracker_t() : depth(0) {}
    depth_tracker_t(int _depth) : depth(_depth) {}

    std::unique_ptr<cookie_t> clone() const override {
      return std::make_unique<depth_tracker_t>(depth);
    }
  };

  root->visit_nodes(
      [node_id, &depth](const Node *node, cookie_t *cookie) {
        depth_tracker_t *depth_tracker = dynamic_cast<depth_tracker_t *>(cookie);

        if (node->get_id() == node_id) {
          depth = depth_tracker->depth;
          return NodeVisitAction::Stop;
        }

        depth_tracker->depth++;
        return NodeVisitAction::Continue;
      },
      std::make_unique<depth_tracker_t>());

  return depth;
}

symbols_t BDD::get_generated_symbols(const Node *node) const {
  symbols_t symbols{device, packet_len, time};

  while (node) {
    if (node->get_type() == NodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const symbols_t &more_symbols = call_node->get_local_symbols();
      symbols.insert(more_symbols.begin(), more_symbols.end());
    }

    node = node->get_prev();
  }

  return symbols;
}

BDD::BDD() : id(0), root(nullptr) {}

BDD::BDD(const call_paths_view_t &call_paths_view)
    : id(0), symbol_manager(call_paths_view.manager) {
  root = bdd_from_call_paths(call_paths_view, symbol_manager, manager, init, id);

  packet_len = symbol_manager->get_symbol("pkt_len");
  time = symbol_manager->get_symbol("next_time");

  // Some NFs don't care about the device, so it doesn't show up on the callpaths.
  if (symbol_manager->has_symbol("DEVICE")) {
    device = call_paths_view.manager->get_symbol("DEVICE");
  } else {
    device = call_paths_view.manager->create_symbol("DEVICE", 32);
  }
}

BDD::BDD(const std::filesystem::path &fpath, SymbolManager *_symbol_manager)
    : id(0), symbol_manager(_symbol_manager) {
  deserialize(fpath);
}

void BDD::assert_integrity() const {
  SYNAPSE_ASSERT(root, "No root node");
  root->visit_nodes([](const Node *node) {
    SYNAPSE_ASSERT(node, "Null node");
    switch (node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(node);
      const Node *on_true = branch->get_on_true();
      const Node *on_false = branch->get_on_false();
      SYNAPSE_ASSERT(on_true, "No on true node");
      SYNAPSE_ASSERT(on_false, "No on false node");
      SYNAPSE_ASSERT(on_true->get_prev() == node, "Invalid on true node");
      SYNAPSE_ASSERT(on_false->get_prev() == node, "Invalid on false node");
      break;
    } break;
    case NodeType::Call:
    case NodeType::Route: {
      if (node->get_next()) {
        SYNAPSE_ASSERT(node->get_next()->get_prev() == node, "Invalid next node");
      }
    } break;
    }
    return NodeVisitAction::Continue;
  });
}

BDD::BDD(const BDD &other)
    : id(other.id), device(other.device), packet_len(other.packet_len), time(other.time),
      init(other.init), symbol_manager(other.symbol_manager) {
  root = other.root->clone(manager, true);
}

BDD::BDD(BDD &&other)
    : id(other.id), device(std::move(other.device)), packet_len(std::move(other.packet_len)),
      time(std::move(other.time)), init(std::move(other.init)), root(other.root),
      manager(std::move(other.manager)), symbol_manager(std::move(other.symbol_manager)) {
  other.root = nullptr;
}

BDD &BDD::operator=(const BDD &other) {
  if (this == &other)
    return *this;
  id = other.id;
  device = other.device;
  packet_len = other.packet_len;
  time = other.time;
  init = other.init;
  root = other.root->clone(manager, true);
  symbol_manager = other.symbol_manager;
  return *this;
}

Node *BDD::delete_non_branch(node_id_t target_id) {
  Node *anchor_next = get_mutable_node_by_id(target_id);
  SYNAPSE_ASSERT(anchor_next, "Node not found");
  SYNAPSE_ASSERT(anchor_next->get_type() != NodeType::Branch, "Unexpected branch node");

  Node *anchor = anchor_next->get_mutable_prev();
  SYNAPSE_ASSERT(anchor, "No previous node");

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

    SYNAPSE_ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection");

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

Node *BDD::delete_branch(node_id_t target_id, bool direction_to_keep) {
  Node *target = get_mutable_node_by_id(target_id);
  SYNAPSE_ASSERT(target, "Node not found");
  SYNAPSE_ASSERT(target->get_type() == NodeType::Branch, "Unexpected branch node");

  Node *anchor = target->get_mutable_prev();
  SYNAPSE_ASSERT(anchor, "No previous node");

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

    SYNAPSE_ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection");

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

Node *BDD::clone_and_add_non_branches(const Node *current,
                                      const std::vector<const Node *> &new_nodes) {
  Node *new_current = nullptr;

  if (new_nodes.empty()) {
    return new_current;
  }

  const Node *prev = current->get_prev();
  SYNAPSE_ASSERT(prev, "No previous node");

  node_id_t anchor_id = prev->get_id();
  Node *anchor = get_mutable_node_by_id(anchor_id);
  Node *anchor_next = get_mutable_node_by_id(current->get_id());

  bool set_new_current = false;

  for (const Node *new_node : new_nodes) {
    SYNAPSE_ASSERT(new_node->get_type() != NodeType::Branch, "Unexpected branch node");

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

      SYNAPSE_ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection found");

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

Branch *BDD::clone_and_add_branch(const Node *current, klee::ref<klee::Expr> condition) {
  const Node *prev = current->get_prev();
  SYNAPSE_ASSERT(prev, "No previous node");

  node_id_t anchor_id = prev->get_id();
  Node *anchor = get_mutable_node_by_id(anchor_id);
  Node *anchor_next = get_mutable_node_by_id(current->get_id());

  klee::ref<klee::Expr> constraint = constraint_from_expr(condition);

  Node *on_true_cond = anchor_next;
  Node *on_false_cond = anchor_next->clone(manager, true);
  on_false_cond->recursive_update_ids(id);

  on_true_cond->recursive_add_constraint(constraint);
  on_false_cond->recursive_add_constraint(solver_toolbox.exprBuilder->Not(constraint));

  Branch *new_branch = create_new_branch(this, current, condition);

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

    SYNAPSE_ASSERT(on_true == anchor_next || on_false == anchor_next, "No connection found");

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

} // namespace synapse