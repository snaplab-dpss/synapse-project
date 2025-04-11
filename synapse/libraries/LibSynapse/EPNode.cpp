#include <LibSynapse/EPNode.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/Tofino/Recirculate.h>
#include <LibCore/Solver.h>

namespace LibSynapse {

namespace {
ep_node_id_t ep_node_id_counter = 0;
}

EPNode::EPNode(Module *_module) : id(ep_node_id_counter++), module(_module), prev(nullptr) {}

EPNode::~EPNode() {
  if (module) {
    delete module;
    module = nullptr;
  }

  for (EPNode *child : children) {
    if (child) {
      delete child;
      child = nullptr;
    }
  }
}

void EPNode::set_children(EPNode *next) { children = {next}; }

void EPNode::set_children(klee::ref<klee::Expr> condition, EPNode *on_true, EPNode *on_false) {
  children   = {on_true, on_false};
  constraint = condition;
}

void EPNode::set_prev(EPNode *_prev) { prev = _prev; }

const Module *EPNode::get_module() const { return module; }
Module *EPNode::get_mutable_module() { return module; }

const std::vector<EPNode *> &EPNode::get_children() const { return children; }
EPNode *EPNode::get_prev() const { return prev; }

ep_node_id_t EPNode::get_id() const { return id; }
void EPNode::set_id(ep_node_id_t _id) { id = _id; }

const EPNode *EPNode::get_node_by_id(ep_node_id_t target_id) const {
  const EPNode *target = nullptr;

  visit_nodes([target_id, &target](const EPNode *node) {
    if (node->get_id() == target_id) {
      target = node;
      return EPNodeVisitAction::Stop;
    }
    return EPNodeVisitAction::Continue;
  });

  return target;
}

EPNode *EPNode::get_mutable_node_by_id(ep_node_id_t target_id) {
  EPNode *target = nullptr;

  visit_mutable_nodes([target_id, &target](EPNode *node) {
    if (node->get_id() == target_id) {
      target = node;
      return EPNodeVisitAction::Stop;
    }
    return EPNodeVisitAction::Continue;
  });

  return target;
}

klee::ref<klee::Expr> EPNode::get_constraint() const { return constraint; }

void EPNode::set_constraint(klee::ref<klee::Expr> _constraint) { constraint = _constraint; }

std::vector<klee::ref<klee::Expr>> EPNode::get_constraints() const {
  std::vector<klee::ref<klee::Expr>> constraints;
  const EPNode *node = prev;
  const EPNode *next = this;

  while (node) {
    klee::ref<klee::Expr> constraint = node->get_constraint();

    if (!constraint.isNull()) {
      if (node->get_children().size() > 1 && node->get_children()[1] == next) {
        constraint = LibCore::solver_toolbox.exprBuilder->Not(constraint);
      }

      constraints.insert(constraints.begin(), constraint);
    }

    next = node;
    node = node->get_prev();
  }

  return constraints;
}

std::string EPNode::dump() const {
  std::stringstream ss;
  ss << "[" << id << "|" << module->get_target() << "] " << module->get_name();
  return ss.str();
}

EPNode *EPNode::clone(bool recursive) const {
  EPNode *cloned_node = new EPNode(module->clone());

  // The constructor increments the ID, let's fix that
  cloned_node->id = id;

  if (recursive) {
    std::vector<EPNode *> children_clones;

    for (const EPNode *child : children) {
      EPNode *cloned_children = child->clone(true);
      cloned_children->prev   = cloned_node;
      children_clones.push_back(cloned_children);
    }

    cloned_node->children = children_clones;
  }

  cloned_node->constraint = constraint;

  return cloned_node;
}

void EPNode::visit_nodes(std::function<EPNodeVisitAction(const EPNode *)> fn) const {
  std::vector<const EPNode *> nodes{this};
  while (nodes.size()) {
    const EPNode *node = nodes[0];
    nodes.erase(nodes.begin());

    EPNodeVisitAction action = fn(node);

    if (action == EPNodeVisitAction::Stop)
      return;

    if (action == EPNodeVisitAction::SkipChildren)
      continue;

    const std::vector<EPNode *> &children = node->get_children();
    nodes.insert(nodes.end(), children.begin(), children.end());
  }
}

void EPNode::visit_mutable_nodes(std::function<EPNodeVisitAction(EPNode *)> fn) {
  std::vector<EPNode *> nodes{this};
  while (nodes.size()) {
    EPNode *node = nodes[0];
    nodes.erase(nodes.begin());

    EPNodeVisitAction action = fn(node);

    if (action == EPNodeVisitAction::Stop)
      return;

    if (action == EPNodeVisitAction::SkipChildren)
      continue;

    const std::vector<EPNode *> &children = node->get_children();
    nodes.insert(nodes.end(), children.begin(), children.end());
  }
}

std::vector<u16> EPNode::get_past_recirculations() const {
  std::vector<u16> past_recirculations;

  const EPNode *node = this;
  while ((node = node->get_prev())) {
    const Module *module = node->get_module();

    if (!module) {
      continue;
    }

    if (module->get_type() == ModuleType::Tofino_Recirculate) {
      const Tofino::Recirculate *recirc_module = dynamic_cast<const Tofino::Recirculate *>(module);
      past_recirculations.push_back(recirc_module->get_recirc_port());
    }
  }

  return past_recirculations;
}

bool EPNode::forwarding_decision_already_made() const {
  const EPNode *node = this;
  while ((node = node->get_prev())) {
    const Module *module = node->get_module();

    if (!module) {
      continue;
    }

    if (module->get_type() == ModuleType::Tofino_Forward || module->get_type() == ModuleType::Tofino_Drop ||
        module->get_type() == ModuleType::Tofino_Broadcast) {
      return true;
    }
  }

  return false;
}

} // namespace LibSynapse