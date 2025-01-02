#include "tofino.h"
#include "tofino_context.h"

#include "../../execution_plan/execution_plan.h"
#include "../module.h"

#include "../../visualizers/ep_visualizer.h"

#include <algorithm>

namespace synapse {
namespace tofino {

namespace {
const Node *get_last_parser_state_op(const EP *ep, std::optional<bool> &direction) {
  EPLeaf leaf = ep->get_active_leaf();

  const EPNode *node = leaf.node;
  const EPNode *next = nullptr;

  while (node) {
    const Module *module = node->get_module();
    SYNAPSE_ASSERT(module, "Module not found");

    if (module->get_type() == ModuleType::Tofino_ParserCondition) {
      SYNAPSE_ASSERT(next && next->get_module(), "Next node not found");
      const Module *next_module = next->get_module();

      if (next_module->get_type() == ModuleType::Tofino_Then) {
        direction = true;
      } else {
        direction = false;
      }

      return module->get_node();
    }

    if (module->get_type() == ModuleType::Tofino_ParserExtraction) {
      return module->get_node();
    }

    next = node;
    node = node->get_prev();
  }

  return nullptr;
}

const EPNode *get_ep_node_from_bdd_node(const EP *ep, const Node *node) {
  std::vector<EPLeaf> active_leaves = ep->get_active_leaves();
  std::vector<const EPNode *> ep_nodes;

  for (const EPLeaf &leaf : active_leaves) {
    if (leaf.node) {
      ep_nodes.push_back(leaf.node);
    }
  }

  while (!ep_nodes.empty()) {
    const EPNode *ep_node = ep_nodes[0];
    ep_nodes.erase(ep_nodes.begin());

    const Module *module = ep_node->get_module();
    SYNAPSE_ASSERT(module, "Module not found");

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

const EPNode *get_ep_node_leaf_from_future_bdd_node(const EP *ep, const Node *node) {
  const std::vector<EPLeaf> &active_leaves = ep->get_active_leaves();

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

TofinoContext::TofinoContext(const toml::table &config) : tna(config) {}

TofinoContext::TofinoContext(const TofinoContext &other) : tna(other.tna) {
  for (const auto &kv : other.obj_to_ds) {
    std::unordered_set<DS *> new_ds;
    for (const auto &ds : kv.second) {
      DS *clone = ds->clone();
      new_ds.insert(clone);
      id_to_ds[clone->id] = clone;
    }
    obj_to_ds[kv.first] = new_ds;
  }
}

TofinoContext::~TofinoContext() {
  std::unordered_set<const DS *> freed;
  for (auto &kv : obj_to_ds) {
    for (auto &ds : kv.second) {
      if (freed.find(ds) != freed.end()) {
        continue;
      }
      delete ds;
      freed.insert(ds);
    }
    kv.second.clear();
  }
  obj_to_ds.clear();
  id_to_ds.clear();
}

bool TofinoContext::has_ds(addr_t addr) const {
  return obj_to_ds.find(addr) != obj_to_ds.end();
}

bool TofinoContext::has_ds(DS_ID id) const { return id_to_ds.find(id) != id_to_ds.end(); }

const std::unordered_set<DS *> &TofinoContext::get_ds(addr_t addr) const {
  auto found_it = obj_to_ds.find(addr);
  SYNAPSE_ASSERT(found_it != obj_to_ds.end(), "Data structure not found");
  return found_it->second;
}

void TofinoContext::save_ds(addr_t addr, DS *ds) {
  auto found_it = id_to_ds.find(ds->id);

  if (found_it != id_to_ds.end()) {
    SYNAPSE_ASSERT(found_it->second->id == ds->id, "Data structure ID mismatch");
    DS *old = found_it->second;
    id_to_ds.erase(ds->id);
    obj_to_ds[addr].erase(old);
    delete old;
  }

  obj_to_ds[addr].insert(ds);
  id_to_ds[ds->id] = ds;
}

const DS *TofinoContext::get_ds_from_id(DS_ID id) const {
  auto it = id_to_ds.find(id);
  SYNAPSE_ASSERT(it != id_to_ds.end(), "Data structure %s not found", id.c_str());
  return it->second;
}

void TofinoContext::parser_select(const EP *ep, const Node *node,
                                  klee::ref<klee::Expr> field,
                                  const std::vector<int> &values, bool negate) {
  node_id_t id = node->get_id();

  std::optional<bool> direction;
  const Node *last_op = get_last_parser_state_op(ep, direction);

  if (!last_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.add_select(id, field, values, negate);
    return;
  }

  node_id_t leaf_id = last_op->get_id();
  tna.parser.add_select(leaf_id, id, field, values, direction, negate);
}

void TofinoContext::parser_transition(const EP *ep, const Node *node,
                                      klee::ref<klee::Expr> hdr) {
  node_id_t id = node->get_id();

  std::optional<bool> direction;
  const Node *last_op = get_last_parser_state_op(ep, direction);

  if (!last_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.add_extract(id, hdr);
    return;
  }

  node_id_t leaf_id = last_op->get_id();
  tna.parser.add_extract(leaf_id, id, hdr, direction);
}

void TofinoContext::parser_accept(const EP *ep, const Node *node) {
  node_id_t id = node->get_id();

  std::optional<bool> direction;
  const Node *last_op = get_last_parser_state_op(ep, direction);

  if (!last_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.accept(id);
  } else {
    node_id_t leaf_id = last_op->get_id();
    tna.parser.accept(leaf_id, id, direction);
  }
}

void TofinoContext::parser_reject(const EP *ep, const Node *node) {
  node_id_t id = node->get_id();

  std::optional<bool> direction;
  const Node *last_op = get_last_parser_state_op(ep, direction);

  if (!last_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.reject(id);
  } else {
    node_id_t leaf_id = last_op->get_id();
    tna.parser.reject(leaf_id, id, direction);
  }
}

std::unordered_set<DS_ID> TofinoContext::get_stateful_deps(const EP *ep,
                                                           const Node *node) const {
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

    for (DS_ID ds_id : tofino_module->get_generated_ds()) {
      const DS *ds = tofino_ctx->get_ds_from_id(ds_id);

      if (ds->primitive) {
        deps.insert(ds_id);
      } else {
        for (const std::unordered_set<const DS *> &ds_set : ds->get_internal()) {
          for (const DS *internal_ds : ds_set) {
            SYNAPSE_ASSERT(internal_ds, "Internal DS not found");
            deps.insert(internal_ds->id);
          }
        }
      }
    }

    ep_node = ep_node->get_prev();
  }

  return deps;
}

void TofinoContext::place(EP *ep, const Node *node, addr_t obj, DS *ds) {
  if (has_ds(ds->id)) {
    // Already placed.
    return;
  }

  save_ds(obj, ds);

  std::unordered_set<DS_ID> deps =
      ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);
  tna.place(ds, deps);
}

bool TofinoContext::check_placement(const EP *ep, const Node *node, const DS *ds) const {
  std::unordered_set<DS_ID> deps =
      ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);

  PlacementStatus status = tna.can_place(ds, deps);

  // if (status != PlacementStatus::SUCCESS) {
  //   TargetType target = ep->get_active_target();
  //   Log::dbg() << "[" << target << "] Cannot place ds " << ds->id << " ("
  //              << status << ")\n";
  // }

  return status == PlacementStatus::SUCCESS;
}

bool TofinoContext::check_many_placements(
    const EP *ep, const Node *node,
    const std::vector<std::unordered_set<DS *>> &ds) const {
  std::unordered_set<DS_ID> deps =
      ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);

  PlacementStatus status = tna.can_place_many(ds, deps);

  // if (status != PlacementStatus::SUCCESS) {
  //   TargetType target = ep->get_active_target();
  //   Log::dbg() << "[" << target << "] Cannot place objs (" << status <<
  //   ")\n"; Log::dbg() << "  DS:\n"; for (const auto &ds_list : ds) {
  //     for (const DS *ds : ds_list) {
  //       Log::dbg() << "   * " << ds->id << "\n";
  //     }
  //   }
  //   Log::dbg() << "  Deps:\n";
  //   for (DS_ID dep : deps) {
  //     Log::dbg() << "   * " << dep << "\n";
  //   }
  // }

  return status == PlacementStatus::SUCCESS;
}

void TofinoContext::debug() const {
  Log::dbg() << "\n";
  Log::dbg() << "****** Placements ******\n";
  for (const auto &[addr, ds_set] : obj_to_ds) {
    Log::dbg() << "Object " << addr << ":\n";
    for (const DS *ds : ds_set) {
      Log::dbg() << "  * " << ds->id << "\n";
    }
  }
  Log::dbg() << "************************\n";

  tna.debug();
}

} // namespace tofino

template <>
const tofino::TofinoContext *Context::get_target_ctx<tofino::TofinoContext>() const {
  TargetType type = TargetType::Tofino;
  SYNAPSE_ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<const tofino::TofinoContext *>(target_ctxs.at(type));
}

template <>
tofino::TofinoContext *Context::get_mutable_target_ctx<tofino::TofinoContext>() {
  TargetType type = TargetType::Tofino;
  SYNAPSE_ASSERT(target_ctxs.find(type) != target_ctxs.end(), "No context for target");
  return dynamic_cast<tofino::TofinoContext *>(target_ctxs.at(type));
}
} // namespace synapse