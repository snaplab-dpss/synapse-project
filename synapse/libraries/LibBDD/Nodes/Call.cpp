#include <LibBDD/Nodes/Call.h>
#include <LibBDD/Nodes/Branch.h>
#include <LibBDD/Nodes/NodeManager.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

#include <cassert>

namespace LibBDD {

using LibCore::concat_exprs;
using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_group_t;
using LibCore::expr_groups_t;
using LibCore::expr_to_string;
using LibCore::get_expr_groups;
using LibCore::simplify;
using LibCore::solver_toolbox;

BDDNode *Call::clone(BDDNodeManager &manager, bool recursive) const {
  Call *clone;

  if (recursive && next) {
    BDDNode *next_clone = next->clone(manager, true);
    clone               = new Call(id, next_clone, nullptr, symbol_manager, call, generated_symbols);
    next_clone->set_prev(clone);
  } else {
    clone = new Call(id, symbol_manager, call, generated_symbols);
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

const Symbols &Call::get_local_symbols() const { return generated_symbols; }

symbol_t Call::get_local_symbol(const std::string &base) const {
  assert(!base.empty() && "Empty base");
  assert(!generated_symbols.empty() && "No symbols");

  const Symbols filtered = generated_symbols.filter_by_base(base);

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

    if (!solver_toolbox.are_exprs_always_equal(vb_value, vr_value)) {
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

    if (solver_toolbox.are_exprs_always_equal(vb_value, vr_value)) {
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

  const addr_t vector         = expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  for (const Call *vector_return : get_future_functions({"vector_return"})) {
    const call_t &vr               = vector_return->get_call();
    addr_t vr_vector               = expr_addr_to_obj_addr(vr.args.at("vector").expr);
    klee::ref<klee::Expr> vr_index = vr.args.at("index").expr;

    if (vr_vector == vector && solver_toolbox.are_exprs_always_equal(vr_index, index)) {
      vector_returns.push_back(vector_return);
    }
  }

  return vector_returns;
}

const Call *Call::get_vector_borrow_from_return() const {
  if (call.function_name != "vector_return") {
    return nullptr;
  }

  addr_t vector               = expr_addr_to_obj_addr(call.args.at("vector").expr);
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  const Call *target_vector_borrow = nullptr;

  for (const Call *vector_borrow : get_prev_functions({"vector_borrow"})) {
    const call_t &vb               = vector_borrow->get_call();
    addr_t vb_vector               = expr_addr_to_obj_addr(vb.args.at("vector").expr);
    klee::ref<klee::Expr> vb_index = vb.args.at("index").expr;

    if (vb_vector == vector && solver_toolbox.are_exprs_always_equal(vb_index, index)) {
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

  return get_vector_borrow_value_future_usage() == 0;
}

u32 Call::get_vector_borrow_value_future_usage() const {
  u32 total_usage = 0;

  if (call.function_name != "vector_borrow" || next == nullptr) {
    return total_usage;
  }

  const symbol_t value = get_local_symbol("vector_data");
  const addr_t vector  = expr_addr_to_obj_addr(call.args.at("vector").expr);

  next->visit_nodes([&value, &total_usage, vector](const BDDNode *node) {
    if (node->get_type() == BDDNodeType::Call) {
      const Call *call_node     = dynamic_cast<const Call *>(node);
      const call_t &future_call = call_node->get_call();

      if (future_call.function_name == "vector_return") {
        const addr_t vb_vector = expr_addr_to_obj_addr(future_call.args.at("vector").expr);

        if (vb_vector == vector) {
          return BDDNodeVisitAction::Continue;
        }
      }
    }

    for (const symbol_t &symbol : node->get_used_symbols().get()) {
      if (symbol.name == value.name) {
        total_usage++;
        break;
      }
    }

    return BDDNodeVisitAction::Continue;
  });

  return total_usage;
}

bool Call::is_vector_return_without_modifications() const { return call.function_name == "vector_return" && is_vector_read(); }

branch_direction_t Call::get_map_get_success_check() const {
  branch_direction_t success_check;

  if (call.function_name != "map_get") {
    return success_check;
  }

  klee::ref<klee::Expr> key       = call.args.at("key").in;
  const symbol_t map_has_this_key = get_local_symbol("map_has_this_key");

  klee::ref<klee::Expr> key_not_found_cond =
      solver_toolbox.exprBuilder->Eq(map_has_this_key.expr, solver_toolbox.exprBuilder->Constant(0, map_has_this_key.expr->getWidth()));

  visit_nodes([&success_check, key_not_found_cond](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Branch) {
      return BDDNodeVisitAction::Continue;
    }

    const Branch *branch            = dynamic_cast<const Branch *>(node);
    klee::ref<klee::Expr> condition = branch->get_condition();

    const bool is_key_not_found_cond     = solver_toolbox.are_exprs_always_equal(condition, key_not_found_cond);
    const bool is_not_key_not_found_cond = solver_toolbox.are_exprs_always_equal(solver_toolbox.exprBuilder->Not(condition), key_not_found_cond);

    if (!is_key_not_found_cond && !is_not_key_not_found_cond) {
      return BDDNodeVisitAction::Continue;
    }

    success_check.branch    = branch;
    success_check.direction = !is_key_not_found_cond;

    return BDDNodeVisitAction::Stop;
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

const Call *Call::packet_borrow_from_return() const {
  assert(call.function_name == "packet_return_chunk" && "Unexpected function");

  klee::ref<klee::Expr> chunk_returned = call.args.at("the_chunk").in;

  const std::vector<const Call *> prev_borrows = get_prev_functions({"packet_borrow_next_chunk"});
  const std::vector<const Call *> prev_returns = get_prev_functions({"packet_return_chunk"});

  assert(prev_borrows.size() && "No previous borrows");
  assert(prev_borrows.size() > prev_returns.size() && "No previous borrow");

  return prev_borrows[prev_borrows.size() - 1 - prev_returns.size()];
}

klee::ref<klee::Expr> Call::get_obj() const {
  const std::set<std::string> in_obj_candidates  = {"map", "vector", "tb", "chain", "cht", "cms", "lpm"};
  const std::set<std::string> out_obj_candidates = {"map_out", "vector_out", "tb_out", "chain_out", "cms_out", "lpm_out"};

  for (const std::string &obj_candidate : in_obj_candidates) {
    if (call.args.find(obj_candidate) != call.args.end()) {
      return call.args.at(obj_candidate).expr;
    }
  }

  for (const std::string &obj_candidate : out_obj_candidates) {
    if (call.args.find(obj_candidate) != call.args.end()) {
      return call.args.at(obj_candidate).out;
    }
  }

  return nullptr;
}

bool Call::guess_header_fields_from_packet_borrow(expr_struct_t &header) const {
  if (call.function_name != "packet_borrow_next_chunk") {
    return false;
  }

  klee::ref<klee::Expr> expr = call.extra_vars.at("the_chunk").second;

  return guess_struct_fields_from_expr(expr, header);
}

bool Call::guess_value_fields_from_vector_borrow(expr_struct_t &value_struct) const {
  if (call.function_name != "vector_borrow") {
    return false;
  }

  klee::ref<klee::Expr> expr = call.extra_vars.at("borrowed_cell").second;

  return guess_struct_fields_from_expr(expr, value_struct);
}

bool Call::guess_struct_fields_from_expr(klee::ref<klee::Expr> expr, expr_struct_t &expr_struct) const {
  if (next == nullptr) {
    return false;
  }

  expr_struct.expr = expr;

  const std::vector<expr_group_t> groups = LibCore::get_expr_groups(expr);
  assert(groups.size() == 1 && "Multiple groups found on a single header");
  assert(groups[0].has_symbol && "No symbol found");

  const std::string &target_symbol = groups[0].symbol;
  const bytes_t expr_offset        = groups[0].offset;
  const bytes_t expr_size          = groups[0].size;

  struct field_candidate_t {
    bytes_t offset;
    bytes_t size;
  };

  auto candidates_sorter = [](const field_candidate_t &a, const field_candidate_t &b) { return a.offset < b.offset; };

  auto sorted_candidates_from_groups = [target_symbol, expr_offset, expr_size,
                                        candidates_sorter](const expr_groups_t &expr_groups) -> std::vector<field_candidate_t> {
    std::vector<field_candidate_t> sorted_candidates;

    for (const expr_group_t &group : expr_groups) {
      if (!group.has_symbol || group.symbol != target_symbol) {
        continue;
      }

      if (group.offset >= expr_offset && group.offset < expr_offset + expr_size) {
        const bytes_t offset = group.offset - expr_offset;
        const bytes_t size   = std::min(group.size, expr_offset + expr_size - group.offset);

        sorted_candidates.emplace_back(offset, size);
      }
    }

    std::sort(sorted_candidates.begin(), sorted_candidates.end(), candidates_sorter);

    std::vector<field_candidate_t> sorted_complete_candidates;

    bytes_t offset = 0;
    for (const field_candidate_t &candidate : sorted_candidates) {
      if (candidate.offset == offset) {
        sorted_complete_candidates.push_back(candidate);
        offset += candidate.size;
      } else {
        assert(candidate.offset > offset && "Candidate offset is not greater than current offset");
        const bytes_t size = candidate.offset - offset;
        sorted_complete_candidates.emplace_back(offset, size);
        offset += size;
      }
    }

    if (offset < expr_size) {
      const bytes_t size = expr_size - offset;
      sorted_complete_candidates.emplace_back(offset, size);
    }

    return sorted_complete_candidates;
  };

  auto merge_sorted_candidates = [candidates_sorter](const std::vector<field_candidate_t> &a,
                                                     const std::vector<field_candidate_t> &b) -> std::vector<field_candidate_t> {
    std::vector<field_candidate_t> result;

    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());

    std::sort(result.begin(), result.end(), candidates_sorter);

    size_t i = 0;
    while (i + 1 < result.size()) {
      if (result[i].offset == result[i + 1].offset) {
        const bytes_t max_size = std::max(result[i].size, result[i + 1].size);
        const bytes_t min_size = std::min(result[i].size, result[i + 1].size);

        if (max_size == min_size) {
          result.erase(result.begin() + i + 1);
          continue;
        }

        result[i].size       = min_size;
        result[i + 1].offset = result[i].offset + min_size;
        result[i + 1].size   = max_size - min_size;
        continue;
      }

      if (result[i].offset + result[i].size >= result[i + 1].offset) {
        result[i].size = result[i + 1].offset - result[i].offset;
      }

      i++;
    }

    return result;
  };

  std::vector<field_candidate_t> best_candidates = sorted_candidates_from_groups(groups);

  next->visit_nodes([expr_offset, expr_size, sorted_candidates_from_groups, merge_sorted_candidates, &best_candidates](const BDDNode *node) {
    const std::vector<expr_groups_t> expr_groups = node->get_expr_groups();

    for (const expr_groups_t &subgroup : expr_groups) {
      const std::vector<field_candidate_t> sorted_candidates = sorted_candidates_from_groups(subgroup);
      best_candidates                                        = merge_sorted_candidates(best_candidates, sorted_candidates);
    }

    return BDDNodeVisitAction::Continue;
  });

  for (const field_candidate_t &candidate : best_candidates) {
    klee::ref<klee::Expr> extracted      = solver_toolbox.exprBuilder->Extract(expr, candidate.offset * 8, candidate.size * 8);
    klee::ref<klee::Expr> candidate_expr = simplify(extracted);
    expr_struct.fields.push_back(candidate_expr);
  }

  klee::ref<klee::Expr> reconstructed_expr = concat_exprs(expr_struct.fields);
  if (!solver_toolbox.are_exprs_always_equal(expr, reconstructed_expr)) {
    panic("*** Bug in struct field extraction ***\nHdr: %s\nReconstructed: %s", expr_to_string(expr, true).c_str(),
          expr_to_string(reconstructed_expr, true).c_str());
  }

  return true;
}

} // namespace LibBDD