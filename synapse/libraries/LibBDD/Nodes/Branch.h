#pragma once

#include <LibBDD/Nodes/Node.h>

namespace LibBDD {

using LibCore::SymbolManager;

class BDDNodeManager;

class Branch : public BDDNode {
private:
  BDDNode *on_false;
  klee::ref<klee::Expr> condition;

public:
  Branch(bdd_node_id_t _id, SymbolManager *_symbol_manager, klee::ref<klee::Expr> _condition)
      : BDDNode(_id, BDDNodeType::Branch, _symbol_manager), on_false(nullptr), condition(_condition) {}

  Branch(bdd_node_id_t _id, BDDNode *_prev, SymbolManager *_symbol_manager, BDDNode *_on_true, BDDNode *_on_false, klee::ref<klee::Expr> _condition)
      : BDDNode(_id, BDDNodeType::Branch, _on_true, _prev, _symbol_manager), on_false(_on_false), condition(_condition) {}

  klee::ref<klee::Expr> get_condition() const { return condition; }

  void set_condition(const klee::ref<klee::Expr> &_condition) { condition = _condition; }

  const BDDNode *get_on_true() const { return next; }
  const BDDNode *get_on_false() const { return on_false; }

  std::unordered_set<std::string> get_used_symbols() const { return symbol_t::get_symbols_names(condition); }

  void set_on_true(BDDNode *_on_true) { next = _on_true; }
  void set_on_false(BDDNode *_on_false) { on_false = _on_false; }

  BDDNode *get_mutable_on_true() { return next; }
  BDDNode *get_mutable_on_false() { return on_false; }

  std::vector<const BDDNode *> get_leaves() const override final;
  std::vector<BDDNode *> get_mutable_leaves() override final;
  BDDNode *clone(BDDNodeManager &manager, bool recursive = false) const override final;
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

  const BDDNode *get_success_node() const {
    if (direction) {
      return branch->get_on_true();
    } else {
      return branch->get_on_false();
    }
  }

  const BDDNode *get_failure_node() const {
    if (direction) {
      return branch->get_on_false();
    } else {
      return branch->get_on_true();
    }
  }
};

} // namespace LibBDD