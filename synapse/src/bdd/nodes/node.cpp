#include <iomanip>

#include <llvm/Support/MD5.h>

#include "node.h"
#include "branch.h"
#include "call.h"
#include "manager.h"
#include "../bdd.h"
#include "../../exprs/solver.h"

namespace synapse {
namespace {
call_t rename_call_symbols(SymbolManager *symbol_manager, const call_t &call,
                           const std::unordered_map<std::string, std::string> &translations) {
  call_t renamed_call = call;

  for (auto &arg_pair : renamed_call.args) {
    arg_t &arg = renamed_call.args[arg_pair.first];
    arg.expr = symbol_manager->translate(arg.expr, translations);
    arg.in = symbol_manager->translate(arg.in, translations);
    arg.out = symbol_manager->translate(arg.out, translations);
  }

  for (auto &extra_var_pair : renamed_call.extra_vars) {
    extra_var_t &extra_var = renamed_call.extra_vars[extra_var_pair.first];
    extra_var.first = symbol_manager->translate(extra_var.first, translations);
    extra_var.second = symbol_manager->translate(extra_var.second, translations);
  }

  renamed_call.ret = symbol_manager->translate(renamed_call.ret, translations);
  return renamed_call;
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

bool Node::is_reachable(node_id_t id) const {
  const Node *node = this;
  while (node) {
    if (node->get_id() == id)
      return true;
    node = node->get_prev();
  }
  return false;
}

size_t Node::count_children(bool recursive) const {
  NodeVisitAction action = recursive ? NodeVisitAction::Continue : NodeVisitAction::SkipChildren;
  const Node *self = this;

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
      const Node *on_true = branch_node->get_on_true();
      const Node *on_false = branch_node->get_on_false();
      if (!on_true)
        paths++;
      if (!on_false)
        paths++;
    } break;
    case NodeType::Call:
    case NodeType::Route: {
      const Node *next = node->get_next();
      if (!next)
        paths++;
    } break;
    }
    return NodeVisitAction::Continue;
  });

  return paths;
}

constraints_t Node::get_ordered_branch_constraints() const {
  std::vector<klee::ref<klee::Expr>> constraints;

  const Node *node = this;
  while (node->get_prev()) {
    const Node *prev = node->get_prev();

    if (prev->get_type() != NodeType::Branch) {
      node = prev;
      continue;
    }

    const Branch *branch = dynamic_cast<const Branch *>(prev);
    klee::ref<klee::Expr> condition = branch->get_condition();

    if (branch->get_on_false() == node) {
      condition = solver_toolbox.exprBuilder->Not(condition);
    }

    constraints.insert(constraints.begin(), condition);
    node = prev;
  }

  return constraints;
}

std::string Node::recursive_dump(int lvl) const {
  std::stringstream result;
  std::string pad(lvl * 2, ' ');

  result << pad << dump(true) << "\n";

  switch (type) {
  case NodeType::Branch: {
    const Branch *branch_node = dynamic_cast<const Branch *>(this);
    const Node *on_true = branch_node->get_on_true();
    const Node *on_false = branch_node->get_on_false();
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
    Node *on_true = branch_node->get_mutable_on_true();
    Node *on_false = branch_node->get_mutable_on_false();
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

void Node::recursive_translate_symbol(SymbolManager *symbol_manager, const symbol_t &old_symbol,
                                      const symbol_t &new_symbol) {
  visit_mutable_nodes([symbol_manager, old_symbol, new_symbol](Node *node) -> NodeVisitAction {
    if (node->get_type() != NodeType::Call)
      return NodeVisitAction::Continue;

    Call *call_node = dynamic_cast<Call *>(node);

    const call_t &call = call_node->get_call();
    call_t new_call =
        rename_call_symbols(symbol_manager, call, {{old_symbol.name, new_symbol.name}});
    call_node->set_call(new_call);

    symbols_t generated_symbols = call_node->get_local_symbols();

    auto same_symbol = [old_symbol](const symbol_t &s) { return s.base == old_symbol.base; };

    size_t removed = std::erase_if(generated_symbols, same_symbol);
    if (removed > 0) {
      generated_symbols.insert(new_symbol);
      call_node->set_locally_generated_symbols(generated_symbols);
    }

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

void Node::visit_nodes(std::function<NodeVisitAction(const Node *, cookie_t *)> fn,
                       std::unique_ptr<cookie_t> cookie) const {
  std::vector<std::pair<const Node *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    const Node *node = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    NodeVisitAction action = fn(node, node_cookie.get());

    if (action == NodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      const Node *on_true = branch_node->get_on_true();
      const Node *on_false = branch_node->get_on_false();

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
      const Node *next = node->get_next();
      if (next && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({next, std::move(node_cookie)});
      }
    } break;
    }
  }
}

void Node::visit_nodes(std::function<NodeVisitAction(const Node *)> fn) const {
  visit_nodes([fn](const Node *node, void *) { return fn(node); }, nullptr);
}

void Node::visit_mutable_nodes(std::function<NodeVisitAction(Node *, cookie_t *)> fn,
                               std::unique_ptr<cookie_t> cookie) {
  std::vector<std::pair<Node *, std::unique_ptr<cookie_t>>> nodes;
  nodes.push_back({this, std::move(cookie)});

  while (nodes.size()) {
    Node *node = nodes[0].first;
    std::unique_ptr<cookie_t> node_cookie = std::move(nodes[0].second);
    nodes.erase(nodes.begin());

    NodeVisitAction action = fn(node, node_cookie.get());

    if (action == NodeVisitAction::Stop) {
      return;
    }

    switch (node->get_type()) {
    case NodeType::Branch: {
      Branch *branch_node = dynamic_cast<Branch *>(node);
      Node *on_true = branch_node->get_mutable_on_true();
      Node *on_false = branch_node->get_mutable_on_false();

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
      Node *next = node->get_mutable_next();
      if (next && action != NodeVisitAction::SkipChildren) {
        std::unique_ptr<cookie_t> new_cookie;
        if (node_cookie) {
          new_cookie = node_cookie->clone();
        }
        nodes.push_back({next, std::move(node_cookie)});
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

    Node *on_true = branch_node->get_mutable_on_true();
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
} // namespace synapse