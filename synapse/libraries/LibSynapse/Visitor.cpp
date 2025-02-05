#include <LibSynapse/Visitor.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Module.h>

#include <iostream>
#include <vector>

namespace LibSynapse {

void EPVisitor::visit(const EP *ep) {
  const EPNode *root = ep->get_root();

  if (root) {
    visit(ep, root);
  }
}

void EPVisitor::visit(const EP *ep, const EPNode *node) {
  log(node);

  const Module *module     = node->get_module();
  EPVisitor::Action action = module->visit(*this, ep, node);

  if (action == EPVisitor::Action::skipChildren) {
    return;
  }

  for (const EPNode *child : node->get_children()) {
    visit(ep, child);
  }
}

void EPVisitor::visit_children(const EP *ep, const EPNode *node) {
  log(node);

  for (const EPNode *child : node->get_children()) {
    visit(ep, child);
  }
}

void EPVisitor::log(const EPNode *node) const {
  const Module *module    = node->get_module();
  const std::string &name = module->get_name();
  TargetType target       = module->get_target();
  std::cerr << "Visiting";
  std::cerr << " EPNode=" << node->get_id();
  std::cerr << " Target=" << target;
  std::cerr << " Name=" << name;
  if (node->get_module()->get_node()) {
    std::cerr << " BDDNode=" << node->get_module()->get_node()->get_id();
  }
  std::cerr << "\n";
}

} // namespace LibSynapse