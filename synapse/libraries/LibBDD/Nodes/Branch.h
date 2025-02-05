#pragma once

#include <LibBDD/Nodes/Node.h>

namespace LibBDD {

class NodeManager;

class Branch : public Node {
private:
  Node *on_false;
  klee::ref<klee::Expr> condition;
  std::unordered_set<std::string> used_symbols;

public:
  Branch(node_id_t _id, const klee::ConstraintManager &_constraints, LibCore::SymbolManager *_symbol_manager,
         klee::ref<klee::Expr> _condition)
      : Node(_id, NodeType::Branch, _constraints, _symbol_manager), on_false(nullptr), condition(_condition),
        used_symbols(LibCore::symbol_t::get_symbols_names(_condition)) {}

  Branch(node_id_t _id, Node *_prev, const klee::ConstraintManager &_constraints, LibCore::SymbolManager *_symbol_manager, Node *_on_true,
         Node *_on_false, klee::ref<klee::Expr> _condition)
      : Node(_id, NodeType::Branch, _on_true, _prev, _constraints, _symbol_manager), on_false(_on_false), condition(_condition),
        used_symbols(LibCore::symbol_t::get_symbols_names(_condition)) {}

  klee::ref<klee::Expr> get_condition() const { return condition; }

  void set_condition(const klee::ref<klee::Expr> &_condition) { condition = _condition; }

  const Node *get_on_true() const { return next; }
  const Node *get_on_false() const { return on_false; }

  const std::unordered_set<std::string> &get_used_symbols() const { return used_symbols; }

  void set_on_true(Node *_on_true) { next = _on_true; }
  void set_on_false(Node *_on_false) { on_false = _on_false; }

  Node *get_mutable_on_true() { return next; }
  Node *get_mutable_on_false() { return on_false; }

  std::vector<node_id_t> get_leaves() const override final;
  Node *clone(NodeManager &manager, bool recursive = false) const override final;
  std::string dump(bool one_liner = false, bool id_name_only = false) const override final;

  // A parser condition should be the single discriminating condition that
  // decides whether a parsing state is performed or not. In BDD language, it
  // decides if a specific packet_borrow_next_chunk is applied.
  //
  // One classic example would be condition that checks if the ethertype field
  // on the ethernet header equals the IP protocol.
  //
  // A branch condition is considered a parsing condition if:
  //   - Has pending chunks to be borrowed in the future
  //   - Only looks at the packet
  bool is_parser_condition() const;
};

struct branch_direction_t {
  const Branch *branch;
  bool direction;

  branch_direction_t() : branch(nullptr), direction(false) {}
  branch_direction_t(const branch_direction_t &other)            = default;
  branch_direction_t(branch_direction_t &&other)                 = default;
  branch_direction_t &operator=(const branch_direction_t &other) = default;
};

} // namespace LibBDD