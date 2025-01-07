#include "call.h"
#include "branch.h"
#include "manager.h"
#include "../bdd.h"
#include "../../util/exprs.h"
#include "../../util/solver.h"

namespace synapse {
Node *Call::clone(NodeManager &manager, bool recursive) const {
  Call *clone;

  if (recursive && next) {
    Node *next_clone = next->clone(manager, true);
    clone = new Call(id, next_clone, nullptr, constraints, symbol_manager, call, generated_symbols);
    next_clone->set_prev(clone);
  } else {
    clone = new Call(id, constraints, symbol_manager, call, generated_symbols);
  }

  manager.add_node(clone);
  return clone;
}

std::string Call::dump(bool one_liner, bool id_name_only) const {
  std::stringstream ss;
  ss << id << ":";
  ss << call.function_name;

  if (id_name_only) {
    return ss.str();
  }

  ss << "(";

  bool first = true;
  for (auto &arg : call.args) {
    if (!first)
      ss << ", ";
    ss << arg.first << ":";
    ss << expr_to_string(arg.second.expr, one_liner);
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "[";
    }
    if (!arg.second.in.isNull()) {
      ss << expr_to_string(arg.second.in, one_liner);
    }
    if (!arg.second.out.isNull()) {
      ss << " -> ";
      ss << expr_to_string(arg.second.out, one_liner);
    }
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "]";
    }
    first = false;
  }
  ss << ")";
  return ss.str();
}

const symbols_t &Call::get_local_symbols() const { return generated_symbols; }

symbol_t Call::get_local_symbol(const std::string &base) const {
  SYNAPSE_ASSERT(!base.empty(), "Empty base");
  SYNAPSE_ASSERT(!generated_symbols.empty(), "No symbols");

  for (const symbol_t &symbol : generated_symbols) {
    if (symbol.base == base) {
      return symbol;
    }
  }

  SYNAPSE_PANIC("Symbol %s not found", base.c_str());
}

bool Call::has_local_symbol(const std::string &base) const {
  for (const symbol_t &symbol : generated_symbols) {
    if (symbol.base == base) {
      return true;
    }
  }

  return false;
}

bool Call::is_vector_read() const {
  const Call *vector_borrow = nullptr;
  const Call *vector_return = nullptr;

  if (call.function_name == "vector_borrow") {
    vector_borrow = this;
    vector_return = get_vector_return_from_borrow();
  } else if (call.function_name == "vector_return") {
    vector_borrow = get_vector_borrow_from_return();
    vector_return = this;
  } else {
    return false;
  }

  SYNAPSE_ASSERT(vector_borrow, "Vector borrow not found");
  SYNAPSE_ASSERT(vector_return, "Vector return not found");

  const call_t &vb = vector_borrow->get_call();
  const call_t &vr = vector_return->get_call();

  klee::ref<klee::Expr> vb_value = vb.extra_vars.at("borrowed_cell").second;
  klee::ref<klee::Expr> vr_value = vr.args.at("value").in;

  return solver_toolbox.are_exprs_always_equal(vb_value, vr_value);
}

bool Call::is_vector_write() const {
  return (call.function_name == "vector_borrow" || call.function_name == "vector_return") &&
         !is_vector_read();
}

bool Call::is_vector_borrow_ignored() const {
  if (call.function_name != "vector_borrow") {
    return false;
  }

  if (!next) {
    return true;
  }

  if (!is_vector_read()) {
    return false;
  }

  symbol_t value = get_local_symbol("vector_data_reset");
  bool used = false;

  next->visit_nodes([&value, &used](const Node *node) {
    for (const symbol_t &symbol : node->get_used_symbols()) {
      if (symbol.name == value.name) {
        used = true;
        return NodeVisitAction::Stop;
      }
    }

    return NodeVisitAction::Continue;
  });

  return !used;
}

const Call *Call::get_vector_return_from_borrow() const {
  if (call.function_name != "vector_borrow") {
    return nullptr;
  }

  addr_t vector = expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  for (const Call *vector_return : get_future_functions({"vector_return"})) {
    const call_t &vr = vector_return->get_call();
    addr_t vr_vector = expr_addr_to_obj_addr(vr.args.at("vector").expr);
    klee::ref<klee::Expr> vr_index = vr.args.at("index").expr;

    if (vr_vector == vector && solver_toolbox.are_exprs_always_equal(vr_index, index)) {
      return vector_return;
    }
  }

  return nullptr;
}

const Call *Call::get_vector_borrow_from_return() const {
  if (call.function_name != "vector_return") {
    return nullptr;
  }

  addr_t vector = expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  for (const Call *vector_borrow : get_prev_functions({"vector_borrow"})) {
    const call_t &vb = vector_borrow->get_call();
    addr_t vb_vector = expr_addr_to_obj_addr(vb.args.at("vector").expr);
    klee::ref<klee::Expr> vb_index = vb.args.at("index").expr;

    if (vb_vector == vector && solver_toolbox.are_exprs_always_equal(vb_index, index)) {
      return vector_borrow;
    }
  }

  return nullptr;
}

bool Call::is_vector_return_without_modifications() const {
  return call.function_name == "vector_return" && is_vector_read();
}

branch_direction_t Call::find_branch_checking_index_alloc() const {
  branch_direction_t index_alloc_check;

  if (call.function_name != "dchain_allocate_new_index") {
    return index_alloc_check;
  }

  symbol_t out_of_space = get_local_symbol("out_of_space");
  symbols_t freed_flows_symbols = symbol_manager->get_symbols_with_base("number_of_freed_flows");
  SYNAPSE_ASSERT(freed_flows_symbols.size() <= 1, "Multiple number_of_freed_flows symbols");

  std::unordered_set<std::string> target_names;
  target_names.insert(out_of_space.name);
  for (const symbol_t &symbol : freed_flows_symbols) {
    target_names.insert(symbol.name);
  }

  visit_nodes([&target_names, &index_alloc_check](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch = dynamic_cast<const Branch *>(node);
    const klee::ref<klee::Expr> condition = branch->get_condition();
    const std::unordered_set<std::string> used = symbol_t::get_symbols_names(condition);

    for (const std::string &name : used) {
      if (target_names.find(name) == target_names.end()) {
        return NodeVisitAction::Continue;
      }
    }

    index_alloc_check.branch = branch;
    return NodeVisitAction::Stop;
  });

  if (index_alloc_check.branch) {
    klee::ref<klee::Expr> success_condition = solver_toolbox.exprBuilder->Eq(
        out_of_space.expr, solver_toolbox.exprBuilder->Constant(0, out_of_space.expr->getWidth()));

    if (!freed_flows_symbols.empty()) {
      klee::ref<klee::Expr> freed_flows = freed_flows_symbols.begin()->expr;
      success_condition = solver_toolbox.exprBuilder->Or(
          success_condition,
          solver_toolbox.exprBuilder->Ne(
              freed_flows, solver_toolbox.exprBuilder->Constant(0, freed_flows->getWidth())));
    }

    SYNAPSE_ASSERT(index_alloc_check.branch->get_on_true(), "No on_true");
    SYNAPSE_ASSERT(index_alloc_check.branch->get_on_false(), "No on_false");

    const Node *on_true = index_alloc_check.branch->get_on_true();
    const Node *on_false = index_alloc_check.branch->get_on_false();

    bool success_on_true =
        solver_toolbox.is_expr_always_true(on_true->get_constraints(), success_condition);
    bool success_on_false =
        solver_toolbox.is_expr_always_true(on_false->get_constraints(), success_condition);

    SYNAPSE_ASSERT((success_on_true || success_on_false), "No branch side is successful");
    SYNAPSE_ASSERT((success_on_true ^ success_on_false),
                   "Both branch sides have the same success condition");

    index_alloc_check.direction = success_on_true;
  }

  return index_alloc_check;
}

branch_direction_t Call::get_map_get_success_check() const {
  branch_direction_t success_check;

  if (call.function_name != "map_get") {
    return success_check;
  }

  klee::ref<klee::Expr> key = call.args.at("key").in;
  symbol_t map_has_this_key = get_local_symbol("map_has_this_key");
  klee::ref<klee::Expr> key_not_found_cond = solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  visit_nodes([&success_check, key_not_found_cond](const Node *node) {
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

bool Call::is_hdr_parse_with_var_len() const {
  if (call.function_name != "packet_borrow_next_chunk") {
    return false;
  }

  klee::ref<klee::Expr> length = call.args.at("length").expr;
  return length->getKind() != klee::Expr::Kind::Constant;
}

bool Call::are_map_read_write_counterparts(const Call *map_get, const Call *map_put) {
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

bool Call::is_map_get_followed_by_map_puts_on_miss(std::vector<const Call *> &map_puts) const {
  const call_t &mg_call = call;

  if (mg_call.function_name != "map_get") {
    return false;
  }

  klee::ref<klee::Expr> obj_expr = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);
  symbol_t map_has_this_key = get_local_symbol("map_has_this_key");

  klee::ref<klee::Expr> failed_map_get = solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr,
      solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  std::vector<const Call *> future_map_puts = get_future_functions({"map_put"});

  klee::ref<klee::Expr> value;
  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    SYNAPSE_ASSERT(mp_call.function_name == "map_put", "Unexpected function");

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

} // namespace synapse