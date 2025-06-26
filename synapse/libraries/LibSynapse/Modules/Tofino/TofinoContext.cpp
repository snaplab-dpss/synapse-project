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

std::unordered_set<DS_ID> TofinoContext::get_stateful_deps(const EP *ep, const LibBDD::Node *node) {
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
      const DS *ds = tofino_ctx->get_data_structures().get_ds_from_id(ds_id);

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
  if (data_structures.has(ds->id)) {
    return;
  }

  data_structures.save(obj, ds);

  const std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);
  tna.pipeline.place(ds, deps);
}

bool TofinoContext::check_placement(const EP *ep, const LibBDD::Node *node, const DS *ds) const {
  const std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);
  const PlacementResult result         = tna.pipeline.can_place(ds, deps);

  if (result.status != PlacementStatus::Success) {
    const TargetType target = ep->get_active_target();
    std::cerr << "[" << target << "] Cannot place ds " << ds->id << " (" << result.status << ")\n";

    debug();
    std::cerr << "  DS: " << ds->id << "\n";
    std::cerr << "  Deps:\n";
    for (DS_ID dep : deps) {
      std::cerr << "   * " << dep << "\n";
    }
  }

  return result.status == PlacementStatus::Success;
}

void TofinoContext::debug() const {
  std::cerr << "\n";
  std::cerr << "****** Placements ******\n";
  for (const auto &[addr, ds_set] : data_structures.get_data_per_obj()) {
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