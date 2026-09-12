#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Debug.h>

#include <klee/util/ExprVisitor.h>

#include <algorithm>
#include <cassert>
#include <functional>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibCore::symbol_t;
using LibCore::Symbols;

namespace {

const EPNode *get_ep_node_from_bdd_node(const EP *ep, const BDDNode *node) {
  std::vector<const EPNode *> ep_nodes;
  for (const EPLeaf &leaf : ep->get_active_leaves()) {
    if (leaf.node) {
      ep_nodes.push_back(leaf.node);
    }
  }

  while (!ep_nodes.empty()) {
    const EPNode *ep_node = ep_nodes[0];
    ep_nodes.erase(ep_nodes.begin());

    const Module *module = ep_node->get_module();
    assert(module && "Module not found");

    if (module->get_node() == node) {
      return ep_node;
    }

    const EPNode *prev = ep_node->get_prev();
    if (prev) {
      ep_nodes.push_back(prev);
    }
  }

  return nullptr;
}

const EPNode *get_ep_node_leaf_from_future_bdd_node(const EP *ep, const BDDNode *node) {
  const std::list<EPLeaf> &active_leaves = ep->get_active_leaves();

  while (node) {
    for (const EPLeaf &leaf : active_leaves) {
      if (leaf.next == node) {
        return leaf.node;
      }
    }

    node = node->get_prev();
  }

  return nullptr;
}
} // namespace

void TofinoContext::parser_select(const BDDNode *node, const std::vector<parser_selection_t> &selections, const BDDNode *last_parser_op,
                                  std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.mutate().add_select(id, selections);
    return;
  }

  const bdd_node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.mutate().add_select(leaf_id, id, selections, direction);
}

void TofinoContext::parser_transition(const BDDNode *node, klee::ref<klee::Expr> hdr, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.mutate().add_extract(id, hdr);
    return;
  }

  const bdd_node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.mutate().add_extract(leaf_id, id, hdr, direction);
}

void TofinoContext::parser_accept(const BDDNode *node, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.mutate().accept(id);
  } else {
    const bdd_node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.mutate().accept(leaf_id, id, direction);
  }
}

void TofinoContext::parser_reject(const BDDNode *node, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.mutate().reject(id);
  } else {
    const bdd_node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.mutate().reject(leaf_id, id, direction);
  }
}

namespace {

// The symbols a Tofino module's data structures stand for: its node's own, and for a
// vector_return (the register update fused with its borrow) the borrowed value too.
Symbols get_produced_symbols(const Module *module) {
  const BDDNode *node = module->get_node();
  if (!node || node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const Call *call_node = dynamic_cast<const Call *>(node);
  Symbols symbols       = call_node->get_local_symbols();
  if (call_node->get_call().function_name == "vector_return") {
    if (const Call *vector_borrow = call_node->get_vector_borrow_from_return()) {
      symbols.add(vector_borrow->get_local_symbols());
    }
  }
  return symbols;
}

// Walks the EP back from `node` to the last recirculation or egress crossing (or the last
// non-Tofino module), collecting the primitive data structures of every module `keep` accepts.
// Both boundaries start a fresh dependency chain: a recirculation because it is a new pass, an
// egress crossing because the egress is a second 20-stage pipeline with its own depth budget.
// Stage memory is deliberately not reset, because the two gresses share the physical stages.
// The data structures of the plan's modules `keep` selects, walking up from `ep_node` until
// the last recirculation (what was placed before it is in a previous pass).
std::unordered_set<DS_ID> collect_deps_from(const EP *ep, const EPNode *ep_node, const std::function<bool(const Module *)> &keep) {
  std::unordered_set<DS_ID> deps;

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  while (ep_node) {
    const Module *module = ep_node->get_module();

    if (module->get_target() != TargetType::Tofino) {
      break;
    }

    if (module->get_type() == ModuleType::Tofino_Recirculate || module->get_type() == ModuleType::Tofino_SendToEgress) {
      break;
    }

    const TofinoModule *tofino_module = dynamic_cast<const TofinoModule *>(module);

    if (keep(module)) {
      for (DS_ID ds_id : tofino_module->get_generated_ds()) {
        const DS *ds = tofino_ctx->get_data_structures().get_ds_from_id(ds_id);

        if (ds->primitive) {
          deps.insert(ds_id);
          continue;
        }

        for (const std::unordered_set<DS_ID> &data_structures : ds->get_internal_primitive_ids()) {
          deps.insert(data_structures.begin(), data_structures.end());
        }
      }
    }

    ep_node = ep_node->get_prev();
  }

  return deps;
}

std::unordered_set<DS_ID> collect_deps(const EP *ep, const BDDNode *node, const std::function<bool(const Module *)> &keep) {
  const EPNode *ep_node = get_ep_node_from_bdd_node(ep, node);
  if (!ep_node) {
    ep_node = get_ep_node_leaf_from_future_bdd_node(ep, node);
    if (!ep_node) {
      return {};
    }
  }
  return collect_deps_from(ep, ep_node, keep);
}

} // namespace

std::unordered_set<DS_ID> TofinoContext::get_stateful_deps(const EP *ep, const BDDNode *node) {
  return collect_deps(ep, node, [](const Module *) { return true; });
}

std::unordered_set<DS_ID> TofinoContext::get_dataflow_deps(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> value) {
  const std::unordered_set<std::string> read = symbol_t::get_symbols_names(value);
  return collect_deps(ep, node, [&read](const Module *module) {
    for (const symbol_t &symbol : get_produced_symbols(module).get()) {
      if (read.count(symbol.name)) {
        return true;
      }
    }
    return false;
  });
}

void TofinoContext::place(EP *ep, const BDDNode *node, addr_t obj, DS *ds) { place(ep, node, obj, ds, get_stateful_deps(ep, node)); }

void TofinoContext::place(EP *ep, const BDDNode *node, addr_t obj, DS *ds, const std::unordered_set<DS_ID> &deps) { place(obj, ds, deps); }

void TofinoContext::place(addr_t obj, DS *ds, const std::unordered_set<DS_ID> &deps) {
  if (!data_structures.has(ds->id)) {
    data_structures.save(obj, std::unique_ptr<DS>(ds));
  }

  tna.pipeline.place(ds, deps);
}

bool TofinoContext::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  return tna.pipeline.can_place(ds, deps) == PlacementStatus::Success;
}

std::optional<TofinoContext::compute_op_plan_t> TofinoContext::plan_compute_op(const std::vector<DS_ID> &run_actions, DS_ID new_action_id,
                                                                               const compute_op_t &op, const std::unordered_set<DS_ID> &deps) const {
  const Pipeline &pipeline   = tna.pipeline;
  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return {};
  }

  const int op_hash_dist_units = (op.kind == ComputeOpKind::Hash) ? ComputeAction::hash_dist_units_for(op.width) : 0;

  int best_stage_id = -1;
  DS_ID best_action_id;
  for (const DS_ID &action_id : run_actions) {
    if (is_shared_compute_action(action_id)) {
      continue; // Called from another path too: an op appended here would run there as well.
    }
    const ComputeAction *action = dynamic_cast<const ComputeAction *>(data_structures.get_ds_from_id(action_id));
    if (!action || !action->can_take(op)) {
      continue;
    }
    const int stage_id = pipeline.get_placed_stage(action_id);
    if (stage_id < soonest_stage_id) {
      continue;
    }
    if (pipeline.resources->stages[stage_id].available_hash_dist_units < op_hash_dist_units) {
      continue;
    }
    if (best_stage_id < 0 || stage_id < best_stage_id) {
      best_stage_id  = stage_id;
      best_action_id = action_id;
    }
  }

  const ComputeAction fresh(new_action_id, 0, {op});
  const int new_stage_id = pipeline.find_stage_for_compute_action(&fresh, deps);

  if (best_stage_id >= 0 && (new_stage_id < 0 || best_stage_id <= new_stage_id)) {
    return compute_op_plan_t{.action_id = best_action_id, .append = true};
  }
  if (new_stage_id >= 0) {
    return compute_op_plan_t{.action_id = new_action_id, .append = false};
  }
  return {};
}

void TofinoContext::append_compute_op(DS_ID action_id, const compute_op_t &op, const std::unordered_set<DS_ID> &deps) {
  const ComputeAction *old = dynamic_cast<const ComputeAction *>(data_structures.get_ds_from_id(action_id));
  assert_or_panic(old, "%s is not a compute action", action_id.c_str());

  std::unique_ptr<ComputeAction> updated = std::make_unique<ComputeAction>(*old);
  updated->ops.push_back(op);

  const int extra_hash_dist_units = updated->get_hash_dist_units() - old->get_hash_dist_units();
  const addr_t obj                = old->obj;

  data_structures.save(obj, std::move(updated));
  tna.pipeline.append_to_compute_action(action_id, extra_hash_dist_units, deps);
}

void TofinoContext::sync_active_leaf(const EP *ep) {
  Gress gress = Gress::Ingress;
  if (ep->has_active_leaf()) {
    for (const EPNode *prev = ep->get_active_leaf().node; prev; prev = prev->get_prev()) {
      const Module *module = prev->get_module();
      if (!module || module->get_target() != TargetType::Tofino) {
        break;
      }
      if (module->get_type() == ModuleType::Tofino_SendToEgress) {
        gress = Gress::Egress;
        break;
      }
      if (module->get_type() == ModuleType::Tofino_Recirculate) {
        break;
      }
    }
  }
  tna.pipeline.set_gress(gress);
}

bool compute_key_t::operator<(const compute_key_t &other) const {
  if (fn != other.fn) {
    return fn < other.fn;
  }
  if (args.size() != other.args.size()) {
    return args.size() < other.args.size();
  }
  for (size_t i = 0; i < args.size(); i++) {
    const int cmp = args[i]->compare(*other.args[i]);
    if (cmp != 0) {
      return cmp < 0;
    }
  }
  return false;
}

namespace {

// Reads of an aliased symbol become reads of the original's array.
class AliasApplier : public klee::ExprVisitor::ExprVisitor {
private:
  const std::unordered_map<std::string, const klee::Array *> &aliases;

public:
  AliasApplier(const std::unordered_map<std::string, const klee::Array *> &_aliases) : klee::ExprVisitor::ExprVisitor(true), aliases(_aliases) {}

  Action visitRead(const klee::ReadExpr &e) override {
    auto found_it = aliases.find(e.updates.root->name);
    if (found_it == aliases.end()) {
      return Action::doChildren();
    }
    klee::UpdateList ul(found_it->second, e.updates.head);
    return Action::changeTo(solver_toolbox.exprBuilder->Read(ul, e.index));
  }
};

// The array a symbol read reads.
class ArrayFinder : public klee::ExprVisitor::ExprVisitor {
public:
  const klee::Array *array = nullptr;

  ArrayFinder() : klee::ExprVisitor::ExprVisitor(true) {}

  Action visitRead(const klee::ReadExpr &e) override {
    if (!array) {
      array = e.updates.root;
    }
    return Action::skipChildren();
  }
};

compute_key_t key_of(const compute_op_t &op) { return compute_key_t{op.fn, op.args}; }

compute_key_t shape_of(const compute_op_t &op) {
  compute_key_t shape{op.fn, {}};
  for (klee::ref<klee::Expr> arg : op.args) {
    shape.args.push_back(LibCore::blank_plain_leaves(arg));
  }
  return shape;
}

// A read of every byte of a symbol (not of the packet): what a shared field can stand for.
bool is_whole_symbol_read(klee::ref<klee::Expr> expr) {
  std::string name;
  if (!LibCore::is_readLSB(expr, name) || name.empty() || name == "packet_chunks") {
    return false;
  }
  ArrayFinder finder;
  finder.visit(expr);
  return finder.array && finder.array->size * 8 == expr->getWidth();
}

} // namespace

klee::ref<klee::Expr> TofinoContext::canonical_value(klee::ref<klee::Expr> value) const {
  if (value.isNull() || compute_reuse->aliases.empty()) {
    return value;
  }
  AliasApplier applier(compute_reuse->aliases);
  return applier.visit(value);
}

compute_op_t TofinoContext::canonical_op(const compute_op_t &op) const {
  compute_op_t canonical = op;
  for (klee::ref<klee::Expr> &arg : canonical.args) {
    arg = canonical_value(arg);
  }
  return canonical;
}

std::optional<compute_reuse_t> TofinoContext::find_reusable_compute_op(const compute_op_t &op) const {
  if (op.fn.empty()) {
    return {};
  }
  auto found_it = compute_reuse->by_key.find(key_of(op));
  if (found_it == compute_reuse->by_key.end()) {
    return {};
  }
  return found_it->second;
}

void TofinoContext::reuse_compute_op(const compute_op_t &op, const compute_reuse_t &original) {
  compute_reuse_state_t &state = compute_reuse.mutate();
  state.reused.insert({op.id, original});
  state.shared.insert(original.action);

  std::string symbol;
  if (!op.out.isNull() && !original.op.out.isNull() && LibCore::is_readLSB(op.out, symbol)) {
    ArrayFinder finder;
    finder.visit(original.op.out);
    assert_or_panic(finder.array, "The original op's output is not a symbol read");
    state.aliases.insert({symbol, finder.array});
  }
}

void TofinoContext::register_compute_op(const compute_op_t &op, DS_ID action, const std::unordered_set<DS_ID> &pass_actions) {
  if (op.fn.empty()) {
    return;
  }
  compute_reuse_state_t &state = compute_reuse.mutate();
  state.by_key.insert({key_of(op), compute_reuse_t{action, op, pass_actions}});
  state.by_shape[shape_of(op)].push_back(key_of(op));
  std::string symbol;
  if (!op.out.isNull() && LibCore::is_readLSB(op.out, symbol)) {
    state.producers.insert({symbol, op.id});
  }
}

std::optional<std::string> TofinoContext::get_producer(klee::ref<klee::Expr> symbol) const {
  std::string name;
  if (symbol.isNull() || !LibCore::is_readLSB(symbol, name)) {
    return {};
  }
  auto found_it = compute_reuse->producers.find(name);
  if (found_it == compute_reuse->producers.end()) {
    return {};
  }
  return found_it->second;
}

std::optional<klee::ref<klee::Expr>> TofinoContext::get_output_alias(const std::string &op_id) const {
  auto found_it = compute_reuse->output_aliases.find(op_id);
  if (found_it == compute_reuse->output_aliases.end()) {
    return {};
  }
  return found_it->second;
}

std::vector<compute_shape_match_t> TofinoContext::find_shape_matches(const compute_op_t &op, const std::unordered_set<DS_ID> &pass_actions,
                                                                     const std::unordered_set<DS_ID> &path_actions) const {
  if (op.fn.empty()) {
    return {};
  }

  // Whether a computed value's producer is among the given pass's actions.
  const auto producer_action_in = [this](klee::ref<klee::Expr> symbol, const std::unordered_set<DS_ID> &pass) -> bool {
    const std::optional<std::string> producer = get_producer(symbol);
    return producer && pass.contains(find_compute_action(*producer));
  };
  // Whether a symbol is one this path computes itself: produced by an action of this pass that
  // no other path shares. A symbol that reached this path by an alias is the other path's, and
  // renaming or overwriting it would clash with that path's own use of it.
  const auto own_fresh = [&](klee::ref<klee::Expr> symbol) -> bool {
    const std::optional<std::string> producer = get_producer(symbol);
    if (!producer) {
      return false;
    }
    const DS_ID action = find_compute_action(*producer);
    return pass_actions.contains(action) && !is_shared_compute_action(action);
  };

  std::vector<std::pair<int, compute_shape_match_t>> candidates; // (stage, match)
  auto shape_it = compute_reuse->by_shape.find(shape_of(op));
  if (shape_it == compute_reuse->by_shape.end()) {
    return {};
  }
  for (const compute_key_t &key : shape_it->second) {
    auto entry_it = compute_reuse->by_key.find(key);
    assert_or_panic(entry_it != compute_reuse->by_key.end(), "A shape index entry with no op");
    const compute_reuse_t &entry = entry_it->second;
    // An action of this very path holds this path's ops, unless it is shared: then its ops are
    // another path's, and this path merely calls it. Two live values of one path cannot share a
    // field, so only the other path's ops are candidates.
    if (key.args.size() != op.args.size() || (path_actions.contains(entry.action) && !is_shared_compute_action(entry.action))) {
      continue;
    }
    std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> pairs;
    bool shape = true;
    for (size_t i = 0; i < key.args.size() && shape; i++) {
      shape = LibCore::same_shape(key.args[i], op.args[i], pairs);
    }
    if (!shape || pairs.empty()) {
      continue;
    }

    // One value on either side maps to one value on the other, or the substitutions below
    // would not produce the placed op's key.
    bool consistent = true;
    for (size_t i = 0; i < pairs.size() && consistent; i++) {
      for (size_t j = i + 1; j < pairs.size() && consistent; j++) {
        const bool same_theirs = pairs[i].first->compare(*pairs[j].first) == 0;
        const bool same_ours   = pairs[i].second->compare(*pairs[j].second) == 0;
        consistent             = same_theirs == same_ours;
      }
    }
    if (!consistent) {
      continue;
    }
    // One value may differ: a chain diverging in one input. Two or more is a different
    // computation, and a move per input to share one op is never worth it.
    std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> distinct;
    for (const auto &pair : pairs) {
      if (std::none_of(distinct.begin(), distinct.end(), [&pair](const auto &d) { return d.first->compare(*pair.first) == 0; })) {
        distinct.push_back(pair);
      }
    }
    if (distinct.size() != 1) {
      continue;
    }
    pairs = distinct;

    compute_shape_match_t match{entry, {}, {}, {}};
    bool ok = true;
    for (const auto &[theirs, ours] : pairs) {
      const bool theirs_computed = is_whole_symbol_read(theirs) && get_producer(theirs).has_value();
      const bool ours_computed   = is_whole_symbol_read(ours) && get_producer(ours).has_value();
      if (theirs_computed && !producer_action_in(theirs, entry.pass_actions)) {
        ok = false; // Carried from an earlier pass on their path: not one field with ours.
      } else if (ours_computed && !own_fresh(ours)) {
        ok = false; // Carried, or the other path's own symbol under an alias.
      } else if (theirs_computed && ours_computed) {
        match.aliased.emplace_back(theirs, ours);
      } else if (ours_computed) {
        const std::optional<compute_move_t> existing = find_compute_move(ours);
        if (existing && existing->from->compare(*theirs) != 0) {
          ok = false; // Their path already moves another value into our field.
        } else {
          match.moved_there.emplace_back(theirs, ours);
        }
      } else if (theirs_computed) {
        match.moved_here.emplace_back(theirs, ours);
      } else {
        ok = false; // Two plain values: would need a field of its own on both paths.
      }
      if (!ok) {
        break;
      }
    }
    if (!ok) {
      continue;
    }

    candidates.emplace_back(tna.pipeline.get_placed_stage(entry.action), match);
  }
  std::stable_sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
    const size_t moves_a = a.second.moved_there.size() + a.second.moved_here.size();
    const size_t moves_b = b.second.moved_there.size() + b.second.moved_here.size();
    return moves_a != moves_b ? moves_a < moves_b : a.first < b.first;
  });
  std::vector<compute_shape_match_t> matches;
  for (auto &[_, match] : candidates) {
    matches.push_back(std::move(match));
  }
  return matches;
}

void TofinoContext::alias_symbol(klee::ref<klee::Expr> symbol, klee::ref<klee::Expr> original_symbol) {
  std::string name;
  if (!LibCore::is_readLSB(symbol, name)) {
    return;
  }
  ArrayFinder finder;
  finder.visit(original_symbol);
  assert_or_panic(finder.array, "The original symbol is not a symbol read");
  const std::optional<std::string> producer = get_producer(symbol);
  assert_or_panic(producer.has_value(), "Aliasing a symbol no op produces");
  compute_reuse_state_t &state = compute_reuse.mutate();
  state.aliases.insert({name, finder.array});
  state.output_aliases.insert({*producer, original_symbol});
}

void TofinoContext::rewrite_compute_op(const std::string &op_id, klee::ref<klee::Expr> from, klee::ref<klee::Expr> to) {
  compute_reuse_state_t &state = compute_reuse.mutate();
  for (auto it = state.by_key.begin(); it != state.by_key.end(); it++) {
    if (it->second.op.id != op_id) {
      continue;
    }
    compute_reuse_t entry   = it->second;
    const compute_key_t old = it->first;
    for (klee::ref<klee::Expr> &arg : entry.op.args) {
      arg = LibCore::substitute_expr(arg, from, to);
    }
    if (entry.op.fn == "rotate_left") {
      entry.op.in_hash = entry.op.in_hash && !LibCore::is_constant(entry.op.args.at(0));
    }
    state.by_key.erase(it);
    state.by_key.insert({key_of(entry.op), entry});
    std::vector<compute_key_t> &shape = state.by_shape[shape_of(entry.op)];
    std::replace_if(
        shape.begin(), shape.end(), [&old](const compute_key_t &k) { return !(k < old) && !(old < k); }, key_of(entry.op));
    // The action's own copy of the op, which the emitter derives the action's calls from.
    const ComputeAction *action = dynamic_cast<const ComputeAction *>(data_structures.get_ds_from_id(entry.action));
    assert_or_panic(action, "%s is not a compute action", entry.action.c_str());
    std::unique_ptr<ComputeAction> updated = std::make_unique<ComputeAction>(*action);
    for (compute_op_t &held : updated->ops) {
      if (held.id == op_id) {
        held = entry.op;
      }
    }
    const addr_t obj = action->obj;
    data_structures.save(obj, std::move(updated));
    break;
  }
  state.rewrites[op_id].emplace_back(from, to);
}

void TofinoContext::add_compute_move(const compute_move_t &move) { compute_reuse.mutate().moves.push_back(move); }

std::optional<compute_move_t> TofinoContext::find_compute_move(klee::ref<klee::Expr> to) const {
  for (const compute_move_t &move : compute_reuse->moves) {
    if (move.to->compare(*to) == 0) {
      return move;
    }
  }
  return {};
}

klee::ref<klee::Expr> TofinoContext::apply_rewrites(const std::string &op_id, klee::ref<klee::Expr> expr) const {
  auto found_it = compute_reuse->rewrites.find(op_id);
  if (found_it == compute_reuse->rewrites.end() || expr.isNull()) {
    return expr;
  }
  for (const auto &[from, to] : found_it->second) {
    expr = LibCore::substitute_expr(expr, from, to);
  }
  return expr;
}

std::optional<compute_reuse_t> TofinoContext::get_compute_reuse(const std::string &op_id) const {
  auto found_it = compute_reuse->reused.find(op_id);
  if (found_it == compute_reuse->reused.end()) {
    return {};
  }
  return found_it->second;
}

DS_ID TofinoContext::find_compute_action(const std::string &op_id) const {
  if (const std::optional<compute_reuse_t> reuse = get_compute_reuse(op_id)) {
    return reuse->action;
  }
  for (const auto &[id, ds] : data_structures.get_data_per_id()) {
    const ComputeAction *action = dynamic_cast<const ComputeAction *>(ds);
    if (!action) {
      continue;
    }
    for (const compute_op_t &op : action->ops) {
      if (op.id == op_id) {
        return id;
      }
    }
  }
  panic("No compute action holds op %s", op_id.c_str());
}

bool TofinoContext::can_place_fast(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  return tna.pipeline.can_place_fast(ds, deps) == PlacementStatus::Success;
}

std::unordered_set<DS_ID> TofinoContext::get_dataflow_deps(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> value,
                                                           const speculations_t &speculations) {
  const std::unordered_set<std::string> read = symbol_t::get_symbols_names(value);
  const auto keep                            = [&read](const Module *module) {
    for (const symbol_t &symbol : get_produced_symbols(module).get()) {
      if (read.count(symbol.name)) {
        return true;
      }
    }
    return false;
  };

  // Up the node's own path: the pass's decisions first, then the plan from the leaf of the
  // node's branch (the last plan node on the path, possibly a recirculation). A recirculation
  // ends the pass: a value from an earlier pass travels in the recirculation header, so it
  // constrains nothing.
  std::unordered_set<DS_ID> deps;
  std::unordered_set<std::string> pending = read; // Symbols whose producer we haven't met yet.
  for (const BDDNode *n = node->get_prev(); n && !pending.empty(); n = n->get_prev()) {
    const speculations_t::node_info_t *info = speculations.find_node_info(n->get_id());
    if (!info) {
      if (const EPNode *leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
        const std::unordered_set<DS_ID> plan_deps = collect_deps_from(ep, leaf, keep);
        deps.insert(plan_deps.begin(), plan_deps.end());
      }
      break;
    }
    for (const auto &[symbol, ds] : info->produced) {
      if (pending.erase(symbol)) {
        deps.insert(ds);
      }
    }
    if (info->recirculated) {
      break;
    }
  }
  return deps;
}

bool TofinoContext::can_place(const EP *ep, const BDDNode *node, const DS *ds) const { return can_place(ep, node, ds, get_stateful_deps(ep, node)); }

bool TofinoContext::can_place(const EP *ep, const BDDNode *node, const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  const PlacementStatus status = tna.pipeline.can_place(ds, deps);

  if (status != PlacementStatus::Success && LibCore::dbg_mode_active) {
    std::cerr << "[" << ep->get_active_target() << "] Cannot place ds " << ds->id << " with deps=[";
    for (const DS_ID &dep : deps) {
      std::cerr << dep << ",";
    }
    std::cerr << " ] (reason=" << status << ")\n";
  }

  return status == PlacementStatus::Success;
}

void TofinoContext::debug() const {
  data_structures.debug();
  tna.debug();
}

} // namespace Tofino

template <> const Tofino::TofinoContext *Context::get_target_ctx<Tofino::TofinoContext>() const {
  const TargetType type = TargetType::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<const Tofino::TofinoContext *>(target_ctxs.at(type));
}

template <> const Tofino::TofinoContext *Context::get_target_ctx_if_available<Tofino::TofinoContext>() const {
  const TargetType type = TargetType::Tofino;
  if (target_ctxs.find(type) == target_ctxs.end()) {
    return nullptr;
  }
  return dynamic_cast<const Tofino::TofinoContext *>(target_ctxs.at(type));
}

template <> Tofino::TofinoContext *Context::get_mutable_target_ctx<Tofino::TofinoContext>() {
  const TargetType type = TargetType::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<Tofino::TofinoContext *>(target_ctxs.at(type));
}

} // namespace LibSynapse