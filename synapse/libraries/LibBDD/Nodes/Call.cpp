#include <LibBDD/Nodes/Call.h>
#include <LibBDD/Nodes/Branch.h>
#include <LibBDD/Nodes/NodeManager.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>

#include <cassert>

namespace LibBDD {

Node *Call::clone(NodeManager &manager, bool recursive) const {
  Call *clone;

  if (recursive && next) {
    Node *next_clone = next->clone(manager, true);
    clone            = new Call(id, next_clone, nullptr, constraints, symbol_manager, call, generated_symbols);
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
    ss << LibCore::expr_to_string(arg.second.expr, one_liner);
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "[";
    }
    if (!arg.second.in.isNull()) {
      ss << LibCore::expr_to_string(arg.second.in, one_liner);
    }
    if (!arg.second.out.isNull()) {
      ss << " -> ";
      ss << LibCore::expr_to_string(arg.second.out, one_liner);
    }
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "]";
    }
    first = false;
  }
  ss << ")";
  return ss.str();
}

const LibCore::Symbols &Call::get_local_symbols() const { return generated_symbols; }

LibCore::symbol_t Call::get_local_symbol(const std::string &base) const {
  assert(!base.empty() && "Empty base");
  assert(!generated_symbols.empty() && "No symbols");

  LibCore::Symbols filtered = generated_symbols.filter_by_base(base);

  assert(!filtered.empty() && "Symbol not found");
  assert(filtered.size() == 1 && "Multiple symbols found");

  return filtered.get().front();
}

bool Call::has_local_symbol(const std::string &base) const { return !generated_symbols.filter_by_base(base).empty(); }

bool Call::is_vector_read() const {
  const Call *vector_borrow = nullptr;
  std::vector<const Call *> vector_returns;

  if (call.function_name == "vector_borrow") {
    vector_borrow  = this;
    vector_returns = get_vector_returns_from_borrow();
  } else if (call.function_name == "vector_return") {
    vector_borrow  = get_vector_borrow_from_return();
    vector_returns = {this};
  } else {
    return false;
  }

  assert(vector_borrow && "Vector borrow not found");

  if (vector_returns.empty()) {
    return true;
  }

  for (const Call *vector_return : vector_returns) {
    const call_t &vb = vector_borrow->get_call();
    const call_t &vr = vector_return->get_call();

    klee::ref<klee::Expr> vb_value = vb.extra_vars.at("borrowed_cell").second;
    klee::ref<klee::Expr> vr_value = vr.args.at("value").in;

    if (!LibCore::solver_toolbox.are_exprs_always_equal(vb_value, vr_value)) {
      return false;
    }
  }

  return true;
}

bool Call::is_vector_write() const {
  const Call *vector_borrow = nullptr;
  std::vector<const Call *> vector_returns;

  if (call.function_name == "vector_borrow") {
    vector_borrow  = this;
    vector_returns = get_vector_returns_from_borrow();
  } else if (call.function_name == "vector_return") {
    vector_borrow  = get_vector_borrow_from_return();
    vector_returns = {this};
  } else {
    return false;
  }

  assert(vector_borrow && "Vector borrow not found");

  if (vector_returns.empty()) {
    return true;
  }

  for (const Call *vector_return : vector_returns) {
    const call_t &vb = vector_borrow->get_call();
    const call_t &vr = vector_return->get_call();

    klee::ref<klee::Expr> vb_value = vb.extra_vars.at("borrowed_cell").second;
    klee::ref<klee::Expr> vr_value = vr.args.at("value").in;

    if (LibCore::solver_toolbox.are_exprs_always_equal(vb_value, vr_value)) {
      return false;
    }
  }

  return true;
}

std::vector<const Call *> Call::get_vector_returns_from_borrow() const {
  std::vector<const Call *> vector_returns;

  if (call.function_name != "vector_borrow") {
    return vector_returns;
  }

  addr_t vector               = LibCore::expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  for (const Call *vector_return : get_future_functions({"vector_return"})) {
    const call_t &vr               = vector_return->get_call();
    addr_t vr_vector               = LibCore::expr_addr_to_obj_addr(vr.args.at("vector").expr);
    klee::ref<klee::Expr> vr_index = vr.args.at("index").expr;

    if (vr_vector == vector && LibCore::solver_toolbox.are_exprs_always_equal(vr_index, index)) {
      vector_returns.push_back(vector_return);
    }
  }

  return vector_returns;
}

const Call *Call::get_vector_borrow_from_return() const {
  if (call.function_name != "vector_return") {
    return nullptr;
  }

  addr_t vector               = LibCore::expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  const Call *target_vector_borrow = nullptr;

  for (const Call *vector_borrow : get_prev_functions({"vector_borrow"})) {
    const call_t &vb               = vector_borrow->get_call();
    addr_t vb_vector               = LibCore::expr_addr_to_obj_addr(vb.args.at("vector").expr);
    klee::ref<klee::Expr> vb_index = vb.args.at("index").expr;

    if (vb_vector == vector && LibCore::solver_toolbox.are_exprs_always_equal(vb_index, index)) {
      assert(!target_vector_borrow && "Multiple vector_borrows for the same vector_return");
      target_vector_borrow = vector_borrow;
    }
  }

  return target_vector_borrow;
}

bool Call::is_vector_borrow_value_ignored() const {
  if (call.function_name != "vector_borrow") {
    return false;
  }

  if (!next) {
    return true;
  }

  if (!is_vector_read()) {
    return false;
  }

  LibCore::symbol_t value = get_local_symbol("vector_data");
  bool used               = false;

  next->visit_nodes([&value, &used](const Node *node) {
    for (const LibCore::symbol_t &symbol : node->get_used_symbols().get()) {
      if (symbol.name == value.name) {
        used = true;
        return NodeVisitAction::Stop;
      }
    }

    return NodeVisitAction::Continue;
  });

  return !used;
}

bool Call::is_vector_return_without_modifications() const { return call.function_name == "vector_return" && is_vector_read(); }

branch_direction_t Call::find_branch_checking_index_alloc() const {
  branch_direction_t index_alloc_check;

  if (call.function_name != "dchain_allocate_new_index") {
    return index_alloc_check;
  }

  assert(false && "TODO: Reimplement this");
  LibCore::symbol_t not_out_of_space   = get_local_symbol("not_out_of_space");
  LibCore::Symbols freed_flows_symbols = symbol_manager->get_symbols_with_base("number_of_freed_flows");
  assert(freed_flows_symbols.size() <= 1 && "Multiple number_of_freed_flows symbols");

  std::unordered_set<std::string> target_names;
  target_names.insert(not_out_of_space.name);
  for (const LibCore::symbol_t &symbol : freed_flows_symbols.get()) {
    target_names.insert(symbol.name);
  }

  visit_nodes([&target_names, &index_alloc_check](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch                       = dynamic_cast<const Branch *>(node);
    const klee::ref<klee::Expr> condition      = branch->get_condition();
    const std::unordered_set<std::string> used = LibCore::symbol_t::get_symbols_names(condition);

    for (const std::string &name : used) {
      if (target_names.find(name) == target_names.end()) {
        return NodeVisitAction::Continue;
      }
    }

    index_alloc_check.branch = branch;
    return NodeVisitAction::Stop;
  });

  if (index_alloc_check.branch) {
    klee::ref<klee::Expr> success_condition = LibCore::solver_toolbox.exprBuilder->Ne(
        not_out_of_space.expr, LibCore::solver_toolbox.exprBuilder->Constant(0, not_out_of_space.expr->getWidth()));

    if (!freed_flows_symbols.empty()) {
      klee::ref<klee::Expr> freed_flows = freed_flows_symbols.get().begin()->expr;
      success_condition                 = LibCore::solver_toolbox.exprBuilder->Or(
                          success_condition,
                          LibCore::solver_toolbox.exprBuilder->Ne(freed_flows, LibCore::solver_toolbox.exprBuilder->Constant(0, freed_flows->getWidth())));
    }

    assert(index_alloc_check.branch->get_on_true() && "No on_true");
    assert(index_alloc_check.branch->get_on_false() && "No on_false");

    const Node *on_true  = index_alloc_check.branch->get_on_true();
    const Node *on_false = index_alloc_check.branch->get_on_false();

    bool success_on_true  = LibCore::solver_toolbox.is_expr_always_true(on_true->get_constraints(), success_condition);
    bool success_on_false = LibCore::solver_toolbox.is_expr_always_true(on_false->get_constraints(), success_condition);

    assert((success_on_true || success_on_false) && "No branch side is successful");
    assert((success_on_true ^ success_on_false) && "Both branch sides have the same success condition");

    index_alloc_check.direction = success_on_true;
  }

  return index_alloc_check;
}

branch_direction_t Call::get_map_get_success_check() const {
  branch_direction_t success_check;

  if (call.function_name != "map_get") {
    return success_check;
  }

  klee::ref<klee::Expr> key                = call.args.at("key").in;
  LibCore::symbol_t map_has_this_key       = get_local_symbol("map_has_this_key");
  klee::ref<klee::Expr> key_not_found_cond = LibCore::solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr, LibCore::solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  visit_nodes([&success_check, key_not_found_cond](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch            = dynamic_cast<const Branch *>(node);
    klee::ref<klee::Expr> condition = branch->get_condition();

    bool is_key_not_found_cond = LibCore::solver_toolbox.are_exprs_always_equal(condition, key_not_found_cond);
    bool is_not_key_not_found_cond =
        LibCore::solver_toolbox.are_exprs_always_equal(LibCore::solver_toolbox.exprBuilder->Not(condition), key_not_found_cond);

    if (!is_key_not_found_cond && !is_not_key_not_found_cond) {
      return NodeVisitAction::SkipChildren;
    }

    success_check.branch    = branch;
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

  addr_t get_obj = LibCore::expr_addr_to_obj_addr(get_obj_expr);
  addr_t put_obj = LibCore::expr_addr_to_obj_addr(put_obj_expr);

  if (get_obj != put_obj) {
    return false;
  }

  klee::ref<klee::Expr> get_key = get.args.at("key").in;
  klee::ref<klee::Expr> put_key = put.args.at("key").in;

  if (!LibCore::solver_toolbox.are_exprs_always_equal(get_key, put_key)) {
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
  klee::ref<klee::Expr> key      = mg_call.args.at("key").in;

  addr_t obj                         = LibCore::expr_addr_to_obj_addr(obj_expr);
  LibCore::symbol_t map_has_this_key = get_local_symbol("map_has_this_key");

  klee::ref<klee::Expr> failed_map_get = LibCore::solver_toolbox.exprBuilder->Eq(
      map_has_this_key.expr, LibCore::solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  std::vector<const Call *> future_map_puts = get_future_functions({"map_put"});

  klee::ref<klee::Expr> value;
  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> map_expr = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> mp_key   = mp_call.args.at("key").in;
    klee::ref<klee::Expr> mp_value = mp_call.args.at("value").expr;

    addr_t map = LibCore::expr_addr_to_obj_addr(map_expr);

    if (map != obj) {
      continue;
    }

    if (!LibCore::solver_toolbox.are_exprs_always_equal(key, mp_key)) {
      return false;
    }

    if (value.isNull()) {
      value = mp_value;
    } else if (!LibCore::solver_toolbox.are_exprs_always_equal(value, mp_value)) {
      return false;
    }

    klee::ConstraintManager constraints = map_put->get_constraints();
    if (!LibCore::solver_toolbox.is_expr_always_true(constraints, failed_map_get)) {
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

bool Call::is_tb_tracing_check_followed_by_update_on_true(const Call *&tb_update_and_check) const {
  const Call *tb_is_tracing     = this;
  const call_t &is_tracing_call = tb_is_tracing->get_call();

  if (is_tracing_call.function_name != "tb_is_tracing") {
    return false;
  }

  klee::ref<klee::Expr> tb                   = is_tracing_call.args.at("tb").expr;
  klee::ref<klee::Expr> is_tracing_condition = LibCore::solver_toolbox.exprBuilder->Ne(
      is_tracing_call.ret, LibCore::solver_toolbox.exprBuilder->Constant(0, is_tracing_call.ret->getWidth()));

  std::vector<const Call *> tb_update_and_checks = tb_is_tracing->get_future_functions({"tb_update_and_check"});

  tb_update_and_check = nullptr;
  for (const Call *candidate : tb_update_and_checks) {
    klee::ref<klee::Expr> candidate_tb = candidate->get_call().args.at("tb").expr;
    if (!LibCore::solver_toolbox.are_exprs_always_equal(tb, candidate_tb)) {
      continue;
    }

    klee::ConstraintManager constraints = candidate->get_constraints();
    if (!LibCore::solver_toolbox.is_expr_always_true(constraints, is_tracing_condition)) {
      continue;
    }

    tb_update_and_check = candidate;
    break;
  }

  return tb_update_and_check != nullptr;
}

const Call *Call::packet_borrow_from_return() const {
  assert(call.function_name == "packet_return_chunk" && "Unexpected function");

  klee::ref<klee::Expr> chunk_returned = call.args.at("the_chunk").in;

  std::vector<const Call *> prev_borrows = get_prev_functions({"packet_borrow_next_chunk"});
  std::vector<const Call *> prev_returns = get_prev_functions({"packet_return_chunk"});

  assert(prev_borrows.size() && "No previous borrows");
  assert(prev_borrows.size() > prev_returns.size() && "No previous borrow");

  return prev_borrows[prev_borrows.size() - 1 - prev_returns.size()];
}

klee::ref<klee::Expr> Call::get_obj() const {
  const std::set<std::string> obj_candidates = {"map", "vector", "tb", "dchain", "cht", "cms", "lpm"};

  for (const std::string &obj_candidate : obj_candidates) {
    if (call.args.find(obj_candidate) != call.args.end()) {
      return call.args.at(obj_candidate).expr;
    }
  }

  return nullptr;
}

} // namespace LibBDD