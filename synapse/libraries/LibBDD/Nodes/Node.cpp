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

using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_to_string;
using LibCore::get_expr_groups;
using LibCore::get_expr_groups_from_condition;
using LibCore::get_unique_symbolic_reads;
using LibCore::solver_toolbox;
using LibCore::symbolic_read_t;

namespace {
call_t rename_call_symbols(SymbolManager *symbol_manager, const call_t &call, const std::unordered_map<std::string, std::string> &translations) {
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

std::vector<const Call *> get_unfiltered_coalescing_nodes(const BDDNode *node, const map_coalescing_objs_t &data) {
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

  auto filter_map_objs = [&data](const BDDNode *future_node) {
    assert(future_node->get_type() == BDDNodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(future_node);
    const call_t &call    = call_node->get_call();

    if (call.args.find("map") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
      const addr_t obj               = expr_addr_to_obj_addr(obj_expr);
      if (obj != data.map) {
        return true;
      }
    } else if (call.args.find("vector") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
      const addr_t obj               = expr_addr_to_obj_addr(obj_expr);
      if (data.vectors.find(obj) == data.vectors.end()) {
        return true;
      }
    } else if (call.args.find("chain") != call.args.end()) {
      klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
      const addr_t obj               = expr_addr_to_obj_addr(obj_expr);
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

std::vector<BDDNode *> BDDNode::get_mutable_leaves() {
  if (!next)
    return {this};
  return next->get_mutable_leaves();
}

std::vector<const BDDNode *> BDDNode::get_leaves() const {
  if (!next)
    return {this};
  return next->get_leaves();
}

std::vector<const BDDNode *> BDDNode::get_reachable_nodes() const {
  std::vector<const BDDNode *> children;
  const BDDNode *self = this;
  visit_nodes([&children, self](const BDDNode *node) -> BDDNodeVisitAction {
    if (node == self) {
      return BDDNodeVisitAction::Continue;
    }

    children.push_back(node);
    return BDDNodeVisitAction::Continue;
  });

  return children;
}

std::vector<BDDNode *> BDDNode::get_mutable_reachable_nodes() {
  std::vector<BDDNode *> children;
  BDDNode *self = this;
  visit_mutable_nodes([&children, self](BDDNode *node) -> BDDNodeVisitAction {
    if (node == self) {
      return BDDNodeVisitAction::Continue;
    }

    children.push_back(node);
    return BDDNodeVisitAction::Continue;
  });

  return children;
}

bool BDDNode::is_reachable(bdd_node_id_t node_id) const {
  const BDDNode *node = this;
  while (node) {
    if (node->get_id() == node_id)
      return true;
    node = node->get_prev();
  }
  return false;
}

size_t BDDNode::count_children(bool recursive) const {
  BDDNodeVisitAction action = recursive ? BDDNodeVisitAction::Continue : BDDNodeVisitAction::SkipChildren;
  const BDDNode *self       = this;

  size_t total = 0;
  visit_nodes([&total, action, self](const BDDNode *node) -> BDDNodeVisitAction {
    if (node == self) {
      return BDDNodeVisitAction::Continue;
    }

    total++;
    return action;
  });

  return total;
}

size_t BDDNode::count_code_paths() const {
  size_t paths = 0;

  visit_nodes([&paths](const BDDNode *node) -> BDDNodeVisitAction {
    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      const BDDNode *on_true    = branch_node->get_on_true();
      const BDDNode *on_false   = branch_node->get_on_false();
      if (!on_true)
        paths++;
      if (!on_false)
        paths++;
    } break;
    case BDDNodeType::Call:
    case BDDNodeType::Route: {
      const BDDNode *next_node = node->get_next();
      if (!next_node)
        paths++;
    } break;
    }
    return BDDNodeVisitAction::Continue;
  });

  return paths;
}

std::vector<klee::ref<klee::Expr>> BDDNode::get_ordered_branch_constraints() const {
  std::vector<klee::ref<klee::Expr>> ordered_branch_constraints;

  const BDDNode *node = this;
  while (node->get_prev()) {
    const BDDNode *prev_node = node->get_prev();

    if (prev_node->get_type() != BDDNodeType::Branch) {
      node = prev_node;
      continue;
    }

    const Branch *branch            = dynamic_cast<const Branch *>(prev_node);
    klee::ref<klee::Expr> condition = branch->get_condition();

    if (branch->get_on_false() == node) {
      condition = solver_toolbox.exprBuilder->Not(condition);
    }

    ordered_branch_constraints.insert(ordered_branch_constraints.begin(), condition);
    node = prev_node;
  }

  return ordered_branch_constraints;
}

std::string BDDNode::recursive_dump(int lvl) const {
  std::stringstream result;
  std::string pad(lvl * 2, ' ');

  result << pad << dump(true) << "\n";

  switch (type) {
  case BDDNodeType::Branch: {
    const Branch *branch_node = dynamic_cast<const Branch *>(this);
    const BDDNode *on_true    = branch_node->get_on_true();
    const BDDNode *on_false   = branch_node->get_on_false();
    if (on_true)
      result << on_true->recursive_dump(lvl + 1);
    if (on_false)
      result << on_false->recursive_dump(lvl + 1);
  } break;
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    if (next)
      result << next->recursive_dump(lvl);
  } break;
  }

  return result.str();
}

std::string BDDNode::hash(bool recursive) const {
  std::stringstream input;
  visit_nodes([&input, recursive](const BDDNode *node) -> BDDNodeVisitAction {
    input << ":" << node->get_id();
    return recursive ? BDDNodeVisitAction::Continue : BDDNodeVisitAction::Stop;
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

void BDDNode::recursive_update_ids(bdd_node_id_t &new_id) {
  id = new_id++;
  switch (type) {
  case BDDNodeType::Branch: {
    Branch *branch_node = dynamic_cast<Branch *>(this);
    BDDNode *on_true    = branch_node->get_mutable_on_true();
    BDDNode *on_false   = branch_node->get_mutable_on_false();
    if (on_true)
      on_true->recursive_update_ids(new_id);
    if (on_false)
      on_false->recursive_update_ids(new_id);
  } break;
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    if (next)
      next->recursive_update_ids(new_id);
  } break;
  }
}

const BDDNode *BDDNode::get_node_by_id(bdd_node_id_t node_id) const {
  const BDDNode *target = nullptr;

  visit_nodes([node_id, &target](const BDDNode *node) -> BDDNodeVisitAction {
    if (node->get_id() == node_id) {
      target = node;
      return BDDNodeVisitAction::Stop;
    }
    return BDDNodeVisitAction::Continue;
  });

  return target;
}

BDDNode *BDDNode::get_mutable_node_by_id(bdd_node_id_t node_id) {
  BDDNode *target = nullptr;

  visit_mutable_nodes([node_id, &target](BDDNode *node) -> BDDNodeVisitAction {
    if (node->get_id() == node_id) {
      target = node;
      return BDDNodeVisitAction::Stop;
    }
    return BDDNodeVisitAction::Continue;
  });

  return target;
}

void BDDNode::recursive_translate_symbol(const symbol_t &old_symbol, const symbol_t &new_symbol) {
  visit_mutable_nodes([this, old_symbol, new_symbol](BDDNode *node) -> BDDNodeVisitAction {
    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      Branch *branch_node                 = dynamic_cast<Branch *>(node);
      klee::ref<klee::Expr> condition     = branch_node->get_condition();
      klee::ref<klee::Expr> new_condition = symbol_manager->translate(condition, {{old_symbol.name, new_symbol.name}});
      branch_node->set_condition(new_condition);
    } break;
    case BDDNodeType::Call: {
      Call *call_node = dynamic_cast<Call *>(node);

      const call_t &call = call_node->get_call();
      call_t new_call    = rename_call_symbols(symbol_manager, call, {{old_symbol.name, new_symbol.name}});
      call_node->set_call(new_call);

      Symbols generated_symbols = call_node->get_local_symbols();
      if (generated_symbols.has(old_symbol.name)) {
        generated_symbols.remove(old_symbol.name);
        generated_symbols.add(new_symbol);
        call_node->set_local_symbols(generated_symbols);
      }
    } break;
    case BDDNodeType::Route: {
      Route *route_node                    = dynamic_cast<Route *>(node);
      klee::ref<klee::Expr> dst_device     = route_node->get_dst_device();
      klee::ref<klee::Expr> new_dst_device = symbol_manager->translate(dst_device, {{old_symbol.name, new_symbol.name}});
      route_node->set_dst_device(new_dst_device);
    } break;
    }

    return BDDNodeVisitAction::Continue;
  });
}

void BDDNode::visit(BDDVisitor &visitor) const { visitor.visit(this); }

void BDDNode::visit_nodes(std::function<BDDNodeVisitAction(const BDDNode *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) const {
  std::vector<std::pair<const BDDNode *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    const BDDNode *node                   = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    BDDNodeVisitAction action = fn(node, node_cookie.get());

    if (action == BDDNodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      const BDDNode *on_true    = branch_node->get_on_true();
      const BDDNode *on_false   = branch_node->get_on_false();

      if (on_true && action != BDDNodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_true, std::move(new_cookie)});
      }

      if (on_false && action != BDDNodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_false, std::move(new_cookie)});
      }
    } break;
    case BDDNodeType::Call:
    case BDDNodeType::Route: {
      const BDDNode *next_node = node->get_next();
      if (next_node && action != BDDNodeVisitAction::SkipChildren) {
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

void BDDNode::visit_nodes(std::function<BDDNodeVisitAction(const BDDNode *)> fn) const {
  visit_nodes([fn](const BDDNode *node, void *) { return fn(node); }, nullptr);
}

void BDDNode::visit_mutable_nodes(std::function<BDDNodeVisitAction(BDDNode *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) {
  std::vector<std::pair<BDDNode *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    BDDNode *node                         = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    const BDDNodeVisitAction action = fn(node, node_cookie.get());

    if (action == BDDNodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      Branch *branch_node = dynamic_cast<Branch *>(node);
      BDDNode *on_true    = branch_node->get_mutable_on_true();
      BDDNode *on_false   = branch_node->get_mutable_on_false();

      if (on_true && action != BDDNodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_true, std::move(new_cookie)});
      }

      if (on_false && action != BDDNodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({on_false, std::move(new_cookie)});
      }
    } break;
    case BDDNodeType::Call:
    case BDDNodeType::Route: {
      BDDNode *next_node = node->get_mutable_next();
      if (next_node && action != BDDNodeVisitAction::SkipChildren) {
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

void BDDNode::visit_mutable_nodes(std::function<BDDNodeVisitAction(BDDNode *)> fn) {
  visit_mutable_nodes([fn](BDDNode *node, cookie_t *) { return fn(node); }, nullptr);
}

void BDDNode::recursive_free_children(BDDNodeManager &manager) {
  switch (type) {
  case BDDNodeType::Branch: {
    Branch *branch_node = dynamic_cast<Branch *>(this);

    BDDNode *on_true  = branch_node->get_mutable_on_true();
    BDDNode *on_false = branch_node->get_mutable_on_false();

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
  case BDDNodeType::Call:
  case BDDNodeType::Route: {
    if (next) {
      next->recursive_free_children(manager);
      manager.free_node(next);
      next = nullptr;
    }
  } break;
  }
}

Symbols BDDNode::get_prev_symbols(const bdd_node_ids_t &stop_nodes) const {
  Symbols symbols;
  const BDDNode *node = get_prev();

  const std::unordered_set<std::string> ignoring_symbols{
      "packet_chunks",
  };

  while (node) {
    if (stop_nodes.find(node->get_id()) != stop_nodes.end()) {
      break;
    }

    if (node->get_type() == BDDNodeType::Call) {
      const Call *call_node        = dynamic_cast<const Call *>(node);
      const Symbols &local_symbols = call_node->get_local_symbols();

      for (const symbol_t &symbol : local_symbols.get()) {
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

std::list<const Call *> BDDNode::get_prev_functions(const std::unordered_set<std::string> &wanted, const bdd_node_ids_t &stop_nodes) const {
  std::list<const Call *> prev_functions;

  const BDDNode *node = this;
  while ((node = node->get_prev())) {
    if (node->get_type() == BDDNodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();

      if (wanted.find(call.function_name) != wanted.end()) {
        prev_functions.push_front(call_node);
      }
    }

    if (stop_nodes.find(node->get_id()) != stop_nodes.end()) {
      break;
    }
  }

  return prev_functions;
}

std::vector<const Call *> BDDNode::get_future_functions(const std::unordered_set<std::string> &wanted, bool stop_on_branches) const {
  std::vector<const Call *> functions;

  visit_nodes([&functions, &wanted](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (wanted.find(call.function_name) != wanted.end()) {
      functions.push_back(call_node);
    }

    return BDDNodeVisitAction::Continue;
  });

  return functions;
}

Symbols BDDNode::get_used_symbols() const {
  Symbols symbols;

  switch (type) {
  case BDDNodeType::Branch: {
    const Branch *branch_node                   = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr                  = branch_node->get_condition();
    const std::unordered_set<std::string> names = symbol_t::get_symbols_names(expr);
    for (const std::string &name : names) {
      symbols.add(symbol_manager->get_symbol(name));
    }
  } break;
  case BDDNodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(this);
    const call_t &call    = call_node->get_call();

    for (const auto &[arg_name, arg] : call.args) {
      if (!arg.expr.isNull()) {
        std::unordered_set<std::string> names = symbol_t::get_symbols_names(arg.expr);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }

      if (!arg.in.isNull()) {
        std::unordered_set<std::string> names = symbol_t::get_symbols_names(arg.in);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      if (!extra_var.first.isNull()) {
        std::unordered_set<std::string> names = symbol_t::get_symbols_names(extra_var.first);
        for (const std::string &name : names) {
          symbols.add(symbol_manager->get_symbol(name));
        }
      }
    }
  } break;
  case BDDNodeType::Route: {
    const Route *route_node                     = dynamic_cast<const Route *>(this);
    klee::ref<klee::Expr> dst_device            = route_node->get_dst_device();
    const std::unordered_set<std::string> names = symbol_t::get_symbols_names(dst_device);
    for (const std::string &name : names) {
      symbols.add(symbol_manager->get_symbol(name));
    }
  } break;
  }

  return symbols;
}

symbolic_reads_t BDDNode::get_used_symbolic_reads() const {
  symbolic_reads_t symbolic_reads;

  switch (type) {
  case BDDNodeType::Branch: {
    const Branch *branch_node  = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr = branch_node->get_condition();
    symbolic_reads             = get_unique_symbolic_reads(expr);
  } break;
  case BDDNodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(this);
    const call_t &call    = call_node->get_call();

    for (const auto &[arg_name, arg] : call.args) {
      for (symbolic_read_t read : get_unique_symbolic_reads(arg.expr)) {
        symbolic_reads.insert(read);
      }

      for (symbolic_read_t read : get_unique_symbolic_reads(arg.in)) {
        symbolic_reads.insert(read);
      }
    }

    for (const auto &[extra_var_name, extra_var] : call.extra_vars) {
      for (symbolic_read_t read : get_unique_symbolic_reads(extra_var.first)) {
        symbolic_reads.insert(read);
      }
    }
  } break;
  case BDDNodeType::Route: {
    const Route *route_node = dynamic_cast<const Route *>(this);
    symbolic_reads          = get_unique_symbolic_reads(route_node->get_dst_device());
  } break;
  }

  return symbolic_reads;
}

bool BDDNode::is_packet_drop_code_path() const {
  bool found_drop = false;

  visit_nodes([&found_drop](const BDDNode *node) {
    if (node->get_type() == BDDNodeType::Branch) {
      return BDDNodeVisitAction::Stop;
    }

    if (node->get_type() != BDDNodeType::Route) {
      return BDDNodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(node);
    RouteOp op              = route_node->get_operation();

    found_drop |= (op == RouteOp::Drop);
    return BDDNodeVisitAction::Stop;
  });

  return found_drop;
}

std::vector<const Call *> BDDNode::get_coalescing_nodes_from_key(klee::ref<klee::Expr> target_key, const map_coalescing_objs_t &data) const {
  std::vector<const Call *> filtered_nodes = get_unfiltered_coalescing_nodes(this, data);

  if (filtered_nodes.empty()) {
    return filtered_nodes;
  }

  klee::ref<klee::Expr> index;

  auto filter_map_nodes_and_retrieve_index = [&target_key, &index](const BDDNode *node) {
    assert(node->get_type() == BDDNodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.args.find("key") == call.args.end()) {
      return false;
    }

    klee::ref<klee::Expr> key = call.args.at("key").in;
    bool same_key             = solver_toolbox.are_exprs_always_equal(key, target_key);

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

  auto filter_vectors_nodes = [&index](const BDDNode *node) {
    assert(node->get_type() == BDDNodeType::Call && "Unexpected node type");

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name != "vector_borrow" && call.function_name != "vector_return") {
      return false;
    }

    assert(!index.isNull() && "Index is null");

    klee::ref<klee::Expr> value = call.args.at("index").expr;
    return !solver_toolbox.are_exprs_always_equal(index, value);
  };

  filtered_nodes.erase(std::remove_if(filtered_nodes.begin(), filtered_nodes.end(), filter_vectors_nodes), filtered_nodes.end());

  return filtered_nodes;
}

std::vector<expr_groups_t> BDDNode::get_expr_groups() const {
  std::vector<expr_groups_t> groups;

  switch (type) {
  case BDDNodeType::Branch: {
    const Branch *branch_node  = dynamic_cast<const Branch *>(this);
    klee::ref<klee::Expr> expr = branch_node->get_condition();
    groups.push_back(get_expr_groups_from_condition(expr));
  } break;
  case BDDNodeType::Call: {
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
  case BDDNodeType::Route: {
    const Route *route_node    = dynamic_cast<const Route *>(this);
    klee::ref<klee::Expr> expr = route_node->get_dst_device();
    groups.push_back(LibCore::get_expr_groups(expr));
  } break;
  }

  return groups;
}

std::vector<const Route *> BDDNode::get_future_routing_decisions() const {
  std::vector<const Route *> routes;

  visit_nodes([&routes](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Route) {
      return BDDNodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(node);
    routes.push_back(route_node);

    return BDDNodeVisitAction::Continue;
  });

  return routes;
}

const Route *BDDNode::get_latest_routing_decision() const {
  const BDDNode *node = this;
  while (node) {
    if (node->get_type() == BDDNodeType::Route) {
      return dynamic_cast<const Route *>(node);
    }
    node = node->get_prev();
  }
  return nullptr;
}

Route *BDDNode::get_mutable_latest_routing_decision() {
  BDDNode *node = this;
  while (node) {
    if (node->get_type() == BDDNodeType::Route) {
      return dynamic_cast<Route *>(node);
    }
    node = node->get_mutable_prev();
  }
  return nullptr;
}

bool BDDNode::equals(const BDDNode *other) const {
  if (other == nullptr) {
    return false;
  }

  if (type != other->type) {
    return false;
  }

  switch (type) {
  case BDDNodeType::Call: {
    const Call *this_call_node  = dynamic_cast<const Call *>(this);
    const Call *other_call_node = dynamic_cast<const Call *>(other);

    const call_t &this_call  = this_call_node->get_call();
    const call_t &other_call = other_call_node->get_call();

    if (this_call.function_name != other_call.function_name) {
      return false;
    }

    for (const auto &[arg_name, this_arg] : this_call.args) {
      if (other_call.args.find(arg_name) == other_call.args.end()) {
        return false;
      }

      const arg_t &other_arg = other_call.args.at(arg_name);
      if (!solver_toolbox.are_exprs_always_equal(this_arg.expr, other_arg.expr)) {
        return false;
      }
      if (!solver_toolbox.are_exprs_always_equal(this_arg.in, other_arg.in)) {
        return false;
      }
      if (!solver_toolbox.are_exprs_always_equal(this_arg.out, other_arg.out)) {
        return false;
      }
    }

    for (const auto &[extra_var_name, extra_var] : this_call.extra_vars) {
      if (other_call.extra_vars.find(extra_var_name) == other_call.extra_vars.end()) {
        return false;
      }

      const auto &other_extra_var = other_call.extra_vars.at(extra_var_name);
      if (!solver_toolbox.are_exprs_always_equal(extra_var.first, other_extra_var.first)) {
        return false;
      }
      if (!solver_toolbox.are_exprs_always_equal(extra_var.second, other_extra_var.second)) {
        return false;
      }
    }

    if (!solver_toolbox.are_exprs_always_equal(this_call.ret, other_call.ret)) {
      return false;
    }
  } break;
  case BDDNodeType::Branch: {
    const Branch *this_branch_node  = dynamic_cast<const Branch *>(this);
    const Branch *other_branch_node = dynamic_cast<const Branch *>(other);

    if (!solver_toolbox.are_exprs_always_equal(this_branch_node->get_condition(), other_branch_node->get_condition())) {
      return false;
    }
  } break;
  case BDDNodeType::Route: {
    const Route *this_route_node  = dynamic_cast<const Route *>(this);
    const Route *other_route_node = dynamic_cast<const Route *>(other);

    if (this_route_node->get_operation() != other_route_node->get_operation()) {
      return false;
    }

    if (!solver_toolbox.are_exprs_always_equal(this_route_node->get_dst_device(), other_route_node->get_dst_device())) {
      return false;
    }
  } break;
  }

  return true;
}

} // namespace LibBDD
