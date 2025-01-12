#pragma once

#include <optional>

#include "node.h"

namespace synapse {
struct branch_direction_t;

class Call : public Node {
private:
  call_t call;
  Symbols generated_symbols;

public:
  Call(node_id_t _id, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager, const call_t &_call,
       const Symbols &_generated_symbols)
      : Node(_id, NodeType::Call, _constraints, _symbol_manager), call(_call), generated_symbols(_generated_symbols) {}

  Call(node_id_t _id, Node *_next, Node *_prev, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager,
       call_t _call, const Symbols &_generated_symbols)
      : Node(_id, NodeType::Call, _next, _prev, _constraints, _symbol_manager), call(_call),
        generated_symbols(_generated_symbols) {}

  const call_t &get_call() const { return call; }
  void set_call(const call_t &new_call) { call = new_call; }

  symbol_t get_local_symbol(const std::string &base) const;
  const Symbols &get_local_symbols() const;
  bool has_local_symbol(const std::string &base) const;

  void set_locally_generated_symbols(const Symbols &new_generated_symbols) { generated_symbols = new_generated_symbols; }

  Node *clone(NodeManager &manager, bool recursive = false) const override final;
  std::string dump(bool one_liner = false, bool id_name_only = false) const;

  std::vector<const Call *> get_vector_returns_from_borrow() const;
  const Call *get_vector_borrow_from_return() const;
  bool is_vector_read() const;
  bool is_vector_write() const;
  bool is_vector_borrow_value_ignored() const;
  bool is_vector_return_without_modifications() const;
  branch_direction_t find_branch_checking_index_alloc() const;
  branch_direction_t get_map_get_success_check() const;
  bool is_hdr_parse_with_var_len() const;

  // Tries to find the pattern of a map_get followed by map_puts, but only when
  // the map_get is not successful (i.e. the key is not found).
  // Conditions to meet:
  // (1) Has at least 1 future map_put
  // (2) All map_put happen if the map_get was not successful
  // (3) All map_puts with the target obj also have the same key as the map_get
  bool is_map_get_followed_by_map_puts_on_miss(std::vector<const Call *> &map_puts) const;

  static bool are_map_read_write_counterparts(const Call *map_get, const Call *map_put);
};
} // namespace synapse