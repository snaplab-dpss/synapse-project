#include "visitor.h"
#include "bdd.h"
#include "nodes/nodes.h"
#include "../log.h"

namespace synapse {
void BDDVisitor::visit(const Node *node) {
  if (!node) {
    return;
  }

  switch (node->get_type()) {
  case NodeType::Branch: {
    const Branch *branch = dynamic_cast<const Branch *>(node);
    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    if (visit(branch) == Action::Continue) {
      on_true->visit(*this);
      on_false->visit(*this);
    }
  } break;
  case NodeType::Call: {
    const Call *call = dynamic_cast<const Call *>(node);
    const Node *next = call->get_next();

    if (visit(call) == Action::Continue && next) {
      next->visit(*this);
    }
  } break;
  case NodeType::Route: {
    const Route *route = dynamic_cast<const Route *>(node);
    const Node *next = route->get_next();

    if (visit(route) == Action::Continue && next) {
      next->visit(*this);
    }
  } break;
  }
}

void BDDVisitor::visitRoot(const Node *root) {
  if (!root)
    return;
  root->visit(*this);
}

void BDDVisitor::visit(const BDD *bdd) {
  const Node *root = bdd->get_root();
  assert(root && "No root node");
  visitRoot(root);
}
} // namespace synapse