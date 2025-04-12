#include <LibBDD/Nodes/Node.h>
#include <LibBDD/Nodes/Nodes.h>
#include <LibBDD/Nodes/NodeManager.h>
#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/Visitor.h>
#include <LibBDD/CallPath.h>
#include <LibCore/Solver.h>
#include <LibCore/Expr.h>

#include <iomanip>

#include <llvm/Support/MD5.h>

namespace LibBDD {

namespace {
call_t rename_call_symbols(LibCore::SymbolManager *symbol_manager, const call_t &call,
                           const std::unordered_map<std::string, std::string> &translations) {
  call_t renamed_call = call;

  for (auto &arg_pair : renamed_call.args) {
    arg_t &arg = renamed_call.args[arg_pair.first];
    arg.expr   = symbol_manager->translate(arg.expr, translations);
    arg.in     = symbol_manager->translate(arg.in, translations);
    arg.out    = symbol_manager->translate(arg.out, translations);
  }

  for (auto &extra_var_pair : renamed_call.extra_vars) {
    extra_var_t &extra_var = renamed_call.extra_vars[extra_var_pair.first];
    extra_var.first        = symbol_manager->translate(extra_var.first, translations);
    extra_var.second       = symbol_manager->translate(extra_var.second, translations);
  }

  renamed_call.ret = symbol_manager->translate(renamed_call.ret, translations);
  return renamed_call;
}

std::vector<const Call *> get_unfiltered_coalescing_nodes(const Node *node, const map_coalescing_objs_t &data) {
  const std::unordered_set<std::string> target_functions{
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

  auto filter_map_objs = [&data](const Node *future_node) {
    assert(future_node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(future_node);
    const call_t &call    = call_node->get_call();

    if (call.args.find("map") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
      const addr_t obj               = LibCore::expr_addr_to_obj_addr(obj_expr);
      if (obj != data.map) {
        return true;
      }
    } else if (call.args.find("vector") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
      const addr_t obj               = LibCore::expr_addr_to_obj_addr(obj_expr);
      if (data.vectors.find(obj) == data.vectors.end()) {
        return true;
      }
    } else if (call.args.find("chain") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
      const addr_t obj               = LibCore::expr_addr_to_obj_addr(obj_expr);
      if (obj != data.dchain) {
        return true;
      }
    } else {
      return true;
    }

    return false;
  };

  unfiltered_nodes.erase(std::remove_if(unfiltered_nodes.begin(), unfiltered_nodes.end(), filter_map_objs), unfiltered_nodes.end());

  return unfiltered_nodes;
}
} // namespace

std::vector<node_id_t> Node::get_leaves() const {
  if (!next)
    return std::vector<node_id_t>{id};
  return next->get_leaves();
}

std::vector<const Node *> Node::get_children(bool recursive) const {
  std::vector<const Node *> children;

  const Node *self = this;

  visit_nodes([&children, self](const Node *node) -> NodeVisitAction {
    if (node == self) {
      return NodeVisitAction::Continue;
    }

    children.push_back(node);
    return NodeVisitAction::Continue;
  });

  return children;
}

bool Node::is_reachable(node_id_t node_id) const {
  const Node *node = this;
  while (node) {
    if (node->get_id() == node_id)
      return true;
    node = node->get_prev();
  }
  return false;
}

size_t Node::count_children(bool recursive) const {
  NodeVisitAction action = recursive ? NodeVisitAction::Continue : NodeVisitAction::SkipChildren;
  const Node *self       = this;

  size_t total = 0;
  visit_nodes([&total, action, self](const Node *node) -> NodeVisitAction {
    if (node == self) {
      return NodeVisitAction::Continue;
    }

    total++;
    return action;
  });

  return total;
}

size_t Node::count_code_paths() const {
  size_t paths = 0;

  visit_nodes([&paths](const Node *node) -> NodeVisitAction {
    switch (node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      const Node *on_true       = branch_node->get_on_true();
      const Node *on_false      = branch_node->get_on_false();
      if (!on_true)
        paths++;
      if (!on_false)
        paths++;
    } break;
    case NodeType::Call:
    case NodeType::Route: {
      const Node *next_node = node->get_next();
      if (!next_node)
        paths++;
    } break;
    }
    return NodeVisitAction::Continue;
  });

  return paths;
}

std::vector<klee::ref<klee::Expr>> Node::get_ordered_branch_constraints() const {
  std::vector<klee::ref<klee::Expr>> ordered_branch_constraints;

  const Node *node = this;
  while (node->get_prev()) {
    const Node *prev_node = node->get_prev();

    if (prev_node->get_type() != NodeType::Branch) {
      node = prev_node;
      continue;
    }

    const Branch *branch            = dynamic_cast<const Branch *>(prev_node);
    klee::ref<klee::Expr> condition = branch->get_condition();

    if (branch->get_on_false() == node) {
      condition = LibCore::solver_toolbox.exprBuilder->Not(condition);
    }

    ordered_branch_constraints.insert(ordered_branch_constraints.begin(), condition);
    node = prev_node;
  }

  return ordered_branch_constraints;
}

std::string Node::recursive_dump(int lvl) const {
  std::stringstream result;
  std::string pad(lvl * 2, ' ');

  result << pad << dump(true) << "\n";

  switch (type) {
  case NodeType::Branch: {
    const Branch *branch_node = dynamic_cast<const Branch *>(this);
    const Node *on_true       = branch_node->get_on_true();
    const Node *on_false      = branch_node->get_on_false();
    if (on_true)
      result << on_true->recursive_dump(lvl + 1);
    if (on_false)
      result << on_false->recursive_dump(lvl + 1);
  } break;
  case NodeType::Call:
  case NodeType::Route: {
    if (next)
      result << next->recursive_dump(lvl);
  } break;
  }

  return result.str();
}

std::string Node::hash(bool recursive) const {
  std::stringstream input;
  visit_nodes([&input, recursive](const Node *node) -> NodeVisitAction {
    input << ":" << node->get_id();
    return recursive ? NodeVisitAction::Continue : NodeVisitAction::Stop;
  });

  llvm::MD5 checksum;
  checksum.update(input.str());

  llvm::MD5::MD5Result result;
  checksum.final(result);

  std::stringstream output;
  output << std::hex << std::setfill('0');

  for (u8 byte : result) {
    output << std::hex << std::setw(2) << static_cast<int>(byte);
  }

  return output.str();
}

void Node::recursive_update_ids(node_id_t &new_id) {
  id = new_id++;
  switch (type) {
  case NodeType::Branch: {
    Branch *branch_node = dynamic_cast<Branch *>(this);
    Node *on_true       = branch_node->get_mutable_on_true();
    Node *on_false      = branch_node->get_mutable_on_false();
    if (on_true)
      on_true->recursive_update_ids(new_id);
    if (on_false)
      on_false->recursive_update_ids(new_id);
  } break;
  case NodeType::Call:
  case NodeType::Route: {
    if (next)
      next->recursive_update_ids(new_id);
  } break;
  }
}

const Node *Node::get_node_by_id(node_id_t node_id) const {
  const Node *target = nullptr;

  visit_nodes([node_id, &target](const Node *node) -> NodeVisitAction {
    if (node->get_id() == node_id) {
      target = node;
      return NodeVisitAction::Stop;
    }
    return NodeVisitAction::Continue;
  });

  return target;
}

Node *Node::get_mutable_node_by_id(node_id_t node_id) {
  Node *target = nullptr;

  visit_mutable_nodes([node_id, &target](Node *node) -> NodeVisitAction {
    if (node->get_id() == node_id) {
      target = node;
      return NodeVisitAction::Stop;
    }
    return NodeVisitAction::Continue;
  });

  return target;
}

void Node::recursive_translate_symbol(const LibCore::symbol_t &old_symbol, const LibCore::symbol_t &new_symbol) {
  visit_mutable_nodes([this, old_symbol, new_symbol](Node *node) -> NodeVisitAction {
    switch (node->get_type()) {
    case NodeType::Branch: {
      Branch *branch_node                 = dynamic_cast<Branch *>(node);
      klee::ref<klee::Expr> condition     = branch_node->get_condition();
      klee::ref<klee::Expr> new_condition = symbol_manager->translate(condition, {{old_symbol.name, new_symbol.name}});
      branch_node->set_condition(new_condition);
    } break;
    case NodeType::Call: {
      Call *call_node = dynamic_cast<Call *>(node);

      const call_t &call = call_node->get_call();
      call_t new_call    = rename_call_symbols(symbol_manager, call, {{old_symbol.name, new_symbol.name}});
      call_node->set_call(new_call);

      LibCore::Symbols generated_symbols = call_node->get_local_symbols();
      if (generated_symbols.has(old_symbol.name)) {
        generated_symbols.remove(old_symbol.name);
        generated_symbols.add(new_symbol);
        call_node->set_local_symbols(generated_symbols);
      }
    } break;
    case NodeType::Route: {
      Route *route_node                    = dynamic_cast<Route *>(node);
      klee::ref<klee::Expr> dst_device     = route_node->get_dst_device();
      klee::ref<klee::Expr> new_dst_device = symbol_manager->translate(dst_device, {{old_symbol.name, new_symbol.name}});
      route_node->set_dst_device(new_dst_device);
    } break;
    }

    const klee::ConstraintManager &old_constraints = node->get_constraints();
    klee::ConstraintManager new_constraints;
    for (klee::ref<klee::Expr> constraint : old_constraints) {
      klee::ref<klee::Expr> new_constraint = symbol_manager->translate(constraint, {{old_symbol.name, new_symbol.name}});
      new_constraints.addConstraint(new_constraint);
    }
    node->set_constraints(new_constraints);

    return NodeVisitAction::Continue;
  });
}

void Node::recursive_add_constraint(klee::ref<klee::Expr> constraint) {
  visit_mutable_nodes([constraint](Node *node) -> NodeVisitAction {
    node->constraints.addConstraint(constraint);
    return NodeVisitAction::Continue;
  });
}

void Node::visit(BDDVisitor &visitor) const { visitor.visit(this); }

void Node::visit_nodes(std::function<NodeVisitAction(const Node *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) const {
  std::vector<std::pair<const Node *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    const Node *node                      = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    NodeVisitAction action = fn(node, node_cookie.get());

    if (action == NodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      const Node *on_true       = branch_node->get_on_true();
      const Node *on_false      = branch_node->get_on_false();

      if (on_true && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_true, std::move(new_cookie)});
      }

      if (on_false && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_false, std::move(new_cookie)});
      }
    } break;
    case NodeType::Call:
    case NodeType::Route: {
      const Node *next_node = node->get_next();
      if (next_node && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({next_node, std::move(node_cookie)});
      }
    } break;
    }
  }
}

void Node::visit_nodes(std::function<NodeVisitAction(const Node *)> fn) const {
  visit_nodes([fn](const Node *node, void *) { return fn(node); }, nullptr);
}

void Node::visit_mutable_nodes(std::function<NodeVisitAction(Node *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) {
  std::vector<std::pair<Node *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    Node *node                            = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    const NodeVisitAction action = fn(node, node_cookie.get());

    if (action == NodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case NodeType::Branch: {
      Branch *branch_node = dynamic_cast<Branch *>(node);
      Node *on_true       = branch_node->get_mutable_on_true();
      Node *on_false      = branch_node->get_mutable_on_false();

      if (on_true && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_true, std::move(new_cookie)});
      }

      if (on_false && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_false, std::move(new_cookie)});
      }
    } break;
    case NodeType::Call:
    case NodeType::Route: {
      Node *next_node = node->get_mutable_next();
      if (next_node && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({next_node, std::move(node_cookie)});
      }
    } break;
    }
  }
}

void Node::visit_mutable_nodes(std::function<NodeVisitAction(Node *)> fn) {
  visit_mutable_nodes([fn](Node *node, cookie_t *) { return fn(node); }, nullptr);
}

void Node::recursive_free_children(NodeManager &manager) {
  switch (type) {
  case NodeType::Branch: {
    Branch *branch_node = dynamic_cast<Branch *>(this);

    Node *on_true  = branch_node->get_mutable_on_true();
    Node *on_false = branch_node->get_mutable_on_false();

    if (on_true) {
      on_true->recursive_free_children(manager);
      manager.free_node(on_true);
      on_true = nullptr;
    }

    if (on_false) {
      on_false->recursive_free_children(manager);
      manager.free_node(on_false);
      on_false = nullptr;
    }
  } break;
  case NodeType::Call:
  case NodeType::Route: {
    if (next) {
      next->recursive_free_children(manager);
      manager.free_node(next);
      next = nullptr;
    }
  } break;
  }
}

LibCore::Symbols Node::get_prev_symbols(const node_ids_t &stop_nodes) const {
  LibCore::Symbols symbols;
  const Node *node = get_prev();

  const std::unordered_set<std::string> ignoring_symbols{
      "packet_chunks",
  };

  while (node) {
    if (stop_nodes.find(node->get_id()) != stop_nodes.end()) {
      break;
    }

    if (node->get_type() == NodeType::Call) {
      const Call *call_node                 = dynamic_cast<const Call *>(node);
      const LibCore::Symbols &local_symbols = call_node->get_local_symbols();

      for (const LibCore::symbol_t &symbol : local_symbols.get()) {
        if (ignoring_symbols.find(symbol.base) != ignoring_symbols.end()) {
          continue;
        }

        symbols.add(symbol);
      }
    }

    node = node->get_prev();
  }

  return symbols;
}

std::vector<const Call *> Node::get_prev_functions(const std::unordered_set<std::string> &wanted, const node_ids_t &stop_nodes) const {
  std::vector<const Call *> prev_functions;

  const Node *node = this;
  while ((node = node->get_prev())) {
    if (node->get_type() == NodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();

      if (wanted.find(call.function_name) != wanted.end()) {
        prev_functions.insert(prev_functions.begin(), call_node);
      }
    }

    if (stop_nodes.find(node->get_id()) != stop_nodes.end()) {
      break;
    }
  }

  return prev_functions;
}

std::vector<const Call *> Node::get_future_functions(const std::unordered_set<std::string> &wanted, bool stop_on_branches) const {
  std::vector<const Call *> functions;

  visit_nodes([&functions, &wanted](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (wanted.find(call.function_name) != wanted.end()) {
      functions.push_back(call_node);
    }

    return NodeVisitAction::Continue;
  });

  return functions;
}

LibCore::Symbols Node::get_used_symbols() const {
  LibCore::Symbols symbols;

  switch (type) {
  case NodeType::Branch: {
    const Branch *branch_node                   = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr                  = branch_node->get_condition();
    const std::unordered_set<std::string> names = LibCore::symbol_t::get_symbols_names(expr);
    for (const std::string &name : names) {
      symbols.add(symbol_manager->get_symbol(name));
    }
  } break;
  case NodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(this);
    const call_t &call    = call_node->get_call();

    for (const auto &[arg_name, arg] : call.args) {
      if (!arg.expr.isNull()) {
        std::unordered_set<std::string> names = LibCore::symbol_t::get_symbols_names(arg.expr);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }

      if (!arg.in.isNull()) {
        std::unordered_set<std::string> names = LibCore::symbol_t::get_symbols_names(arg.in);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      if (!extra_var.first.isNull()) {
        std::unordered_set<std::string> names = LibCore::symbol_t::get_symbols_names(extra_var.first);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }
    }
  } break;
  case NodeType::Route: {
    const Route *route_node                     = dynamic_cast<const Route *>(this);
    klee::ref<klee::Expr> dst_device            = route_node->get_dst_device();
    const std::unordered_set<std::string> names = LibCore::symbol_t::get_symbols_names(dst_device);
    for (const std::string &name : names) {
      symbols.add(symbol_manager->get_symbol(name));
    }
  } break;
  }

  return symbols;
}

LibCore::symbolic_reads_t Node::get_used_symbolic_reads() const {
  LibCore::symbolic_reads_t symbolic_reads;

  switch (type) {
  case NodeType::Branch: {
    const Branch *branch_node  = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr = branch_node->get_condition();
    symbolic_reads             = LibCore::get_unique_symbolic_reads(expr);
  } break;
  case NodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(this);
    const call_t &call    = call_node->get_call();

    for (const auto &[arg_name, arg] : call.args) {
      for (LibCore::symbolic_read_t read : LibCore::get_unique_symbolic_reads(arg.expr)) {
        symbolic_reads.insert(read);
      }

      for (LibCore::symbolic_read_t read : LibCore::get_unique_symbolic_reads(arg.in)) {
        symbolic_reads.insert(read);
      }
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      for (LibCore::symbolic_read_t read : LibCore::get_unique_symbolic_reads(extra_var.first)) {
        symbolic_reads.insert(read);
      }
    }
  } break;
  case NodeType::Route: {
    const Route *route_node = dynamic_cast<const Route *>(this);
    symbolic_reads          = LibCore::get_unique_symbolic_reads(route_node->get_dst_device());
  } break;
  }

  return symbolic_reads;
}

bool Node::is_packet_drop_code_path() const {
  bool found_drop = false;

  visit_nodes([&found_drop](const Node *node) {
    if (node->get_type() == NodeType::Branch) {
      return NodeVisitAction::Stop;
    }

    if (node->get_type() != NodeType::Route) {
      return NodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(node);
    RouteOp op              = route_node->get_operation();

    found_drop |= (op == RouteOp::Drop);
    return NodeVisitAction::Stop;
  });

  return found_drop;
}

std::vector<const Call *> Node::get_coalescing_nodes_from_key(klee::ref<klee::Expr> target_key, const map_coalescing_objs_t &data) const {
  std::vector<const Call *> filtered_nodes = get_unfiltered_coalescing_nodes(this, data);

  if (filtered_nodes.empty()) {
    return filtered_nodes;
  }

  klee::ref<klee::Expr> index;

  auto filter_map_nodes_and_retrieve_index = [&target_key, &index](const Node *node) {
    assert(node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.args.find("key") == call.args.end()) {
      return false;
    }

    klee::ref<klee::Expr> key = call.args.at("key").in;
    bool same_key             = LibCore::solver_toolbox.are_exprs_always_equal(key, target_key);

    if (same_key) {
      if (call.function_name == "map_get") {
        index = call.args.at("value_out").out;
      } else if (call.function_name == "map_put") {
        index = call.args.at("value").expr;
      }
    }

    return !same_key;
  };

  filtered_nodes.erase(std::remove_if(filtered_nodes.begin(), filtered_nodes.end(), filter_map_nodes_and_retrieve_index), filtered_nodes.end());

  auto filter_vectors_nodes = [&index](const Node *node) {
    assert(node->get_type() == NodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return false;
    }

    assert(!index.isNull() && "Index is null");

    klee::ref<klee::Expr> value = call.args.at("index").expr;
    return !LibCore::solver_toolbox.are_exprs_always_equal(index, value);
  };

  filtered_nodes.erase(std::remove_if(filtered_nodes.begin(), filtered_nodes.end(), filter_vectors_nodes), filtered_nodes.end());

  return filtered_nodes;
}

std::vector<LibCore::expr_groups_t> Node::get_expr_groups() const {
  std::vector<LibCore::expr_groups_t> groups;

  switch (type) {
  case NodeType::Branch: {
    const Branch *branch_node  = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr = branch_node->get_condition();
    groups.push_back(LibCore::get_expr_groups_from_condition(expr));
  } break;
  case NodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(this);
    const call_t &call    = call_node->get_call();

    for (const auto &[arg_name, arg] : call.args) {
      groups.push_back(LibCore::get_expr_groups(arg.expr));
      groups.push_back(LibCore::get_expr_groups(arg.in));
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      groups.push_back(LibCore::get_expr_groups(extra_var.first));
    }
  } break;
  case NodeType::Route: {
    const Route *route_node    = dynamic_cast<const Route *>(this);
    klee::ref<klee::Expr> expr = route_node->get_dst_device();
    groups.push_back(LibCore::get_expr_groups(expr));
  } break;
  }

  return groups;
}

std::vector<const Route *> Node::get_future_routing_decisions() const {
  std::vector<const Route *> routes;

  visit_nodes([&routes](const Node *node) {
    if (node->get_type() != NodeType::Route) {
      return NodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(node);
    routes.push_back(route_node);

    return NodeVisitAction::Continue;
  });

  return routes;
}

} // namespace LibBDD