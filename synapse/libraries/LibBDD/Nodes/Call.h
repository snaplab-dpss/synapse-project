#pragma once

#include <LibBDD/Nodes/Node.h>
#include <LibBDD/CallPath.h>

#include <optional>

namespace LibBDD {

using LibCore::expr_struct_t;
using LibCore::symbol_t;
using LibCore::SymbolManager;
using LibCore::Symbols;

struct branch_direction_t;

class Call : public BDDNode {
private:
  call_t call;
  Symbols generated_symbols;

public:
  Call(bdd_node_id_t _id, SymbolManager *_symbol_manager, const call_t &_call, const Symbols &_generated_symbols)
      : BDDNode(_id, BDDNodeType::Call, _symbol_manager), call(_call), generated_symbols(_generated_symbols) {}

  Call(bdd_node_id_t _id, BDDNode *_next, BDDNode *_prev, SymbolManager *_symbol_manager, call_t _call, const Symbols &_generated_symbols)
      : BDDNode(_id, BDDNodeType::Call, _next, _prev, _symbol_manager), call(_call), generated_symbols(_generated_symbols) {}

  const call_t &get_call() const { return call; }
  void set_call(const call_t &new_call) { call = new_call; }

  symbol_t get_local_symbol(const std::string &base) const;
  const Symbols &get_local_symbols() const;
  bool has_local_symbol(const std::string &base) const;
  void add_local_symbol(const symbol_t &symbol) { generated_symbols.add(symbol); }
  void set_local_symbols(const Symbols &new_generated_symbols) { generated_symbols = new_generated_symbols; }

  BDDNode *clone(BDDNodeManager &manager, bool recursive = false) const override final;
  std::string dump(bool one_liner = false, bool id_name_only = false) const;

  klee::ref<klee::Expr> get_obj() const;
  std::vector<const Call *> get_vector_returns_from_borrow() const;
  const Call *get_vector_borrow_from_return() const;
  bool is_vector_read() const;
  bool is_vector_write() const;
  bool is_vector_borrow_value_ignored() const;
  u32 get_vector_borrow_value_future_usage() const;
  bool is_vector_return_without_modifications() const;
  branch_direction_t get_map_get_success_check() const;
  bool is_hdr_parse_with_var_len() const;
  const Call *packet_borrow_from_return() const;
  bool guess_header_fields_from_packet_borrow(expr_struct_t &header) const;
  bool guess_value_fields_from_vector_borrow(expr_struct_t &value_struct) const;

  static bool are_map_read_write_counterparts(const Call *map_get, const Call *map_put);

private:
  bool guess_struct_fields_from_expr(klee::ref<klee::Expr> expr, expr_struct_t &expr_struct) const;
};

} // namespace LibBDD