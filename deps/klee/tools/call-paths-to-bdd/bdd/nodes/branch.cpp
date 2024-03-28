#include "branch.h"
#include "../visitor.h"

namespace BDD {

Node_ptr Branch::clone(bool recursive) const {
  Node_ptr clone_on_true, clone_on_false;

  assert(next);
  assert(on_false);

  if (recursive) {
    clone_on_true = next->clone(true);
    clone_on_false = on_false->clone(true);
  } else {
    clone_on_true = next;
    clone_on_false = on_false;
  }

  auto clone = std::make_shared<Branch>(id, clone_on_true, prev, constraints,
                                        clone_on_false, condition);

  if (recursive) {
    clone_on_true->prev = clone;
    clone_on_false->prev = clone;
  }

  return clone;
}

std::vector<node_id_t> Branch::get_terminating_node_ids() const {
  std::vector<node_id_t> terminating_ids;

  assert(next);
  assert(on_false);

  auto on_true_ids = next->get_terminating_node_ids();
  auto on_false_ids = on_false->get_terminating_node_ids();

  terminating_ids.insert(terminating_ids.end(), on_true_ids.begin(),
                         on_true_ids.end());

  terminating_ids.insert(terminating_ids.end(), on_false_ids.begin(),
                         on_false_ids.end());

  return terminating_ids;
}

void Branch::recursive_update_ids(node_id_t &new_id) {
  update_id(new_id);
  new_id++;
  next->recursive_update_ids(new_id);
  on_false->recursive_update_ids(new_id);
}

void Branch::visit(BDDVisitor &visitor) const { visitor.visit(this); }

std::string Branch::dump(bool one_liner) const {
  std::stringstream ss;
  ss << id << ":";
  ss << "if (";
  ss << kutil::expr_to_string(condition, one_liner);
  ss << ")";
  return ss.str();
}

std::string Branch::dump_recursive(int lvl) const {
  std::stringstream result;

  result << Node::dump_recursive(lvl);
  result << on_false->dump_recursive(lvl + 1);

  return result.str();
}

} // namespace BDD