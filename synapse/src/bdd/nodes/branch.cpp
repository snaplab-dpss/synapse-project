#include "branch.h"
#include "manager.h"
#include "../bdd.h"
#include "../../util/exprs.h"
#include "../../system.h"

namespace synapse {
Node *Branch::clone(NodeManager &manager, bool recursive) const {
  Node *clone;

  const Node *on_true  = get_on_true();
  const Node *on_false = get_on_false();

  if (recursive) {
    Node *on_true_clone  = on_true ? on_true->clone(manager, true) : nullptr;
    Node *on_false_clone = on_false ? on_false->clone(manager, true) : nullptr;
    clone = new Branch(id, nullptr, constraints, symbol_manager, on_true_clone, on_false_clone, condition);
    if (on_true_clone)
      on_true_clone->set_prev(clone);
    if (on_false_clone)
      on_false_clone->set_prev(clone);
  } else {
    clone = new Branch(id, constraints, symbol_manager, condition);
  }

  manager.add_node(clone);
  return clone;
}

std::vector<node_id_t> Branch::get_leaves() const {
  std::vector<node_id_t> terminating_ids;

  assert(next && "No on_true node");
  assert(on_false && "No on_false node");

  auto on_true_ids  = next->get_leaves();
  auto on_false_ids = on_false->get_leaves();

  terminating_ids.insert(terminating_ids.end(), on_true_ids.begin(), on_true_ids.end());

  terminating_ids.insert(terminating_ids.end(), on_false_ids.begin(), on_false_ids.end());

  return terminating_ids;
}

std::string Branch::dump(bool one_liner, bool id_name_only) const {
  std::stringstream ss;
  ss << id << ":";
  ss << "if";

  if (id_name_only) {
    return ss.str();
  }

  ss << " (";
  ss << expr_to_string(condition, one_liner);
  return ss.str();
}

bool Branch::is_parser_condition() const {
  std::vector<const Call *> future_borrows = get_future_functions({"packet_borrow_next_chunk"});

  if (future_borrows.empty()) {
    return false;
  }

  const std::vector<std::string> allowed_names{"pkt_len", "packet_chunks", "DEVICE"};
  const std::unordered_set<std::string> names = symbol_t::get_symbols_names(condition);

  for (const std::string &name : names) {
    auto found_it = std::find(allowed_names.begin(), allowed_names.end(), name);
    if (found_it == allowed_names.end()) {
      return false;
    }
  }

  return true;
}

} // namespace synapse