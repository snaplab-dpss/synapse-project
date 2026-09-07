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

// Walks the EP back from `node` to the last recirculation (or the last non-Tofino module),
// collecting the primitive data structures of every module `keep` accepts.
std::unordered_set<DS_ID> collect_deps(const EP *ep, const BDDNode *node, const std::function<bool(const Module *)> &keep) {
  std::unordered_set<DS_ID> deps;

  const EPNode *ep_node = get_ep_node_from_bdd_node(ep, node);
  if (!ep_node) {
    ep_node = get_ep_node_leaf_from_future_bdd_node(ep, node);

    if (!ep_node) {
      return deps;
    }
  }

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  while (ep_node) {
    const Module *module = ep_node->get_module();

    if (module->get_target() != TargetType::Tofino) {
      break;
    }

    if (module->get_type() == ModuleType::Tofino_Recirculate) {
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

bool TofinoContext::can_place_fast(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  return tna.pipeline.can_place_fast(ds, deps) == PlacementStatus::Success;
}

std::unordered_set<DS_ID> TofinoContext::get_dataflow_deps(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> value,
                                                           const speculations_t &speculations) {
  std::unordered_set<DS_ID> deps;
  if (!speculations.recirculated_since_leaf) {
    deps = get_dataflow_deps(ep, node, value);
  }
  for (const std::string &name : symbol_t::get_symbols_names(value)) {
    auto found_it = speculations.producers.find(name);
    if (found_it != speculations.producers.end()) {
      deps.insert(found_it->second);
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