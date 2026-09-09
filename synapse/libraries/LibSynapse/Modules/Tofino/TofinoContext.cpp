#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Debug.h>

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

bool TofinoContext::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const { return tna.pipeline.can_place(ds, deps) == PlacementStatus::Success; }

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

DS_ID TofinoContext::find_compute_action(const std::string &op_id) const {
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