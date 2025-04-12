#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/ExecutionPlan.h>

#include <algorithm>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

namespace {

const EPNode *get_ep_node_from_bdd_node(const EP *ep, const LibBDD::Node *node) {
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

const EPNode *get_ep_node_leaf_from_future_bdd_node(const EP *ep, const LibBDD::Node *node) {
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

TofinoContext::TofinoContext(const tna_config_t &tna_config) : tna(tna_config) {}

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

bool TofinoContext::has_ds(addr_t addr) const { return obj_to_ds.find(addr) != obj_to_ds.end(); }

bool TofinoContext::has_ds(DS_ID id) const { return id_to_ds.find(id) != id_to_ds.end(); }

const std::unordered_set<DS *> &TofinoContext::get_ds(addr_t addr) const {
  auto found_it = obj_to_ds.find(addr);
  if (found_it == obj_to_ds.end()) {
    debug();
    panic("Data structure not found with addr %lu", addr);
  }
  return found_it->second;
}

void TofinoContext::save_ds(addr_t addr, DS *ds) {
  auto found_it = id_to_ds.find(ds->id);

  if (found_it != id_to_ds.end()) {
    assert(found_it->second->id == ds->id && "Data structure ID mismatch");
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
  assert(it != id_to_ds.end() && "Data structure not found");
  return it->second;
}

void TofinoContext::parser_select(const LibBDD::Node *node, const std::vector<parser_selection_t> &selections, const LibBDD::Node *last_parser_op,
                                  std::optional<bool> direction) {
  const LibBDD::node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.add_select(id, selections);
    return;
  }

  const LibBDD::node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.add_select(leaf_id, id, selections, direction);
}

void TofinoContext::parser_transition(const LibBDD::Node *node, klee::ref<klee::Expr> hdr, const LibBDD::Node *last_parser_op,
                                      std::optional<bool> direction) {
  const LibBDD::node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.add_extract(id, hdr);
    return;
  }

  const LibBDD::node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.add_extract(leaf_id, id, hdr, direction);
}

void TofinoContext::parser_accept(const LibBDD::Node *node, const LibBDD::Node *last_parser_op, std::optional<bool> direction) {
  const LibBDD::node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.accept(id);
  } else {
    const LibBDD::node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.accept(leaf_id, id, direction);
  }
}

void TofinoContext::parser_reject(const LibBDD::Node *node, const LibBDD::Node *last_parser_op, std::optional<bool> direction) {
  const LibBDD::node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.reject(id);
  } else {
    const LibBDD::node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.reject(leaf_id, id, direction);
  }
}

std::unordered_set<DS_ID> TofinoContext::get_stateful_deps(const EP *ep, const LibBDD::Node *node) const {
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
            assert(internal_ds && "Internal DS not found");
            deps.insert(internal_ds->id);
          }
        }
      }
    }

    ep_node = ep_node->get_prev();
  }

  return deps;
}

void TofinoContext::place(EP *ep, const LibBDD::Node *node, addr_t obj, DS *ds) {
  if (has_ds(ds->id)) {
    return;
  }

  save_ds(obj, ds);

  std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);
  tna.place(ds, deps);
}

bool TofinoContext::check_placement(const EP *ep, const LibBDD::Node *node, const DS *ds) const {
  std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);

  PlacementStatus status = tna.can_place(ds, deps);

  if (status != PlacementStatus::Success) {
    TargetType target = ep->get_active_target();
    std::cerr << "[" << target << "] Cannot place ds " << ds->id << " (" << status << ")\n";
  }

  return status == PlacementStatus::Success;
}

bool TofinoContext::check_many_placements(const EP *ep, const LibBDD::Node *node,
                                          const std::vector<std::unordered_set<DS *>> &data_structures) const {
  std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);

  PlacementStatus status = tna.can_place_many(data_structures, deps);

  if (status != PlacementStatus::Success) {
    TargetType target = ep->get_active_target();
    std::cerr << "[" << target << "] Cannot place objs (" << status << ")\n";
    std::cerr << "  DS:\n";
    for (const std::unordered_set<LibSynapse::Tofino::DS *> &ds_list : data_structures) {
      for (const DS *ds : ds_list) {
        std::cerr << "   * " << ds->id << "\n";
      }
    }
    std::cerr << "  Deps:\n";
    for (DS_ID dep : deps) {
      std::cerr << "   * " << dep << "\n";
    }
  }

  return status == PlacementStatus::Success;
}

void TofinoContext::debug() const {
  std::cerr << "\n";
  std::cerr << "****** Placements ******\n";
  for (const auto &[addr, ds_set] : obj_to_ds) {
    std::cerr << "Object " << addr << ":\n";
    for (const DS *ds : ds_set) {
      std::cerr << "  * " << ds->id << "\n";
    }
  }
  std::cerr << "************************\n";

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