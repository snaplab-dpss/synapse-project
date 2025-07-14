#include <LibBDD/Visitors/Visitor.h>
#include <LibBDD/BDD.h>

namespace LibBDD {

void BDDVisitor::visit(const BDDNode *node) {
  if (!node) {
    return;
  }

  switch (node->get_type()) {
  case BDDNodeType::Branch: {
    const Branch *branch    = dynamic_cast<const Branch *>(node);
    const BDDNode *on_true  = branch->get_on_true();
    const BDDNode *on_false = branch->get_on_false();

    if (visit(branch) == Action::Continue) {
      on_true->visit(*this);
      on_false->visit(*this);
    }
  } break;
  case BDDNodeType::Call: {
    const Call *call    = dynamic_cast<const Call *>(node);
    const BDDNode *next = call->get_next();

    if (visit(call) == Action::Continue && next) {
      next->visit(*this);
    }
  } break;
  case BDDNodeType::Route: {
    const Route *route  = dynamic_cast<const Route *>(node);
    const BDDNode *next = route->get_next();

    if (visit(route) == Action::Continue && next) {
      next->visit(*this);
    }
  } break;
  }
}

void BDDVisitor::visitRoot(const BDDNode *root) {
  if (!root)
    return;
  root->visit(*this);
}

void BDDVisitor::visit(const BDD *bdd) {
  const BDDNode *root = bdd->get_root();
  assert(root && "No root node");
  visitRoot(root);
}

} // namespace LibBDD