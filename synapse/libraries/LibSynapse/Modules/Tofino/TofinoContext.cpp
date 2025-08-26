#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/ExecutionPlan.h>

#include <algorithm>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

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
    tna.parser.add_select(id, selections);
    return;
  }

  const bdd_node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.add_select(leaf_id, id, selections, direction);
}

void TofinoContext::parser_transition(const BDDNode *node, klee::ref<klee::Expr> hdr, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.add_extract(id, hdr);
    return;
  }

  const bdd_node_id_t leaf_id = last_parser_op->get_id();
  tna.parser.add_extract(leaf_id, id, hdr, direction);
}

void TofinoContext::parser_accept(const BDDNode *node, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.accept(id);
  } else {
    const bdd_node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.accept(leaf_id, id, direction);
  }
}

void TofinoContext::parser_reject(const BDDNode *node, const BDDNode *last_parser_op, std::optional<bool> direction) {
  const bdd_node_id_t id = node->get_id();

  if (!last_parser_op) {
    // No leaf node found, add the initial parser state.
    tna.parser.reject(id);
  } else {
    const bdd_node_id_t leaf_id = last_parser_op->get_id();
    tna.parser.reject(leaf_id, id, direction);
  }
}

std::unordered_set<DS_ID> TofinoContext::get_stateful_deps(const EP *ep, const BDDNode *node) {
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

    if (module->get_target().type != TargetArchitecture::Tofino) {
      break;
    }

    if (module->get_type().type == ModuleCategory::Tofino_Recirculate) {
      break;
    }

    const TofinoModule *tofino_module = dynamic_cast<const TofinoModule *>(module);

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

    ep_node = ep_node->get_prev();
  }

  return deps;
}

void TofinoContext::place(EP *ep, const BDDNode *node, addr_t obj, DS *ds) {
  const Tofino::TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const std::unordered_set<DS_ID> deps    = tofino_ctx->get_stateful_deps(ep, node);

  if (!data_structures.has(ds->id)) {
    data_structures.save(obj, std::unique_ptr<DS>(ds));
  }

  tna.pipeline.place(ds, deps);
}

bool TofinoContext::can_place(const EP *ep, const BDDNode *node, const DS *ds) const {
  const std::unordered_set<DS_ID> deps = ep->get_ctx().get_target_ctx<TofinoContext>()->get_stateful_deps(ep, node);
  const PlacementStatus status         = tna.pipeline.can_place(ds, deps);

  if (status != PlacementStatus::Success) {
    std::cerr << "[" << ep->get_active_target() << "] Cannot place ds " << ds->id << " with deps=[";
    for (const DS_ID &dep : deps) {
      std::cerr << dep << ",";
    }
    std::cerr << " ] (reason=" << status << ")\n";
  }

  return true;
}

void TofinoContext::debug() const {
  data_structures.debug();
  tna.debug();
}

} // namespace Tofino

template <> const Tofino::TofinoContext *Context::get_target_ctx<Tofino::TofinoContext>() const {
  const TargetArchitecture type = TargetArchitecture::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<const Tofino::TofinoContext *>(target_ctxs.at(type));
}

template <> const Tofino::TofinoContext *Context::get_target_ctx_if_available<Tofino::TofinoContext>() const {
  const TargetArchitecture type = TargetArchitecture::Tofino;
  if (target_ctxs.find(type) == target_ctxs.end()) {
    return nullptr;
  }
  return dynamic_cast<const Tofino::TofinoContext *>(target_ctxs.at(type));
}

template <> Tofino::TofinoContext *Context::get_mutable_target_ctx<Tofino::TofinoContext>() {
  const TargetArchitecture type = TargetArchitecture::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end() && "No context for target");
  return dynamic_cast<Tofino::TofinoContext *>(target_ctxs.at(type));
}

} // namespace LibSynapse
