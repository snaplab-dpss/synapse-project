#include "return_raw.h"
#include "../visitor.h"

namespace BDD {

Node_ptr ReturnRaw::clone(bool recursive) const {
  auto clone = std::make_shared<ReturnRaw>(id, prev, constraints, calls_list);
  return clone;
}

void ReturnRaw::recursive_update_ids(node_id_t &new_id) {
  update_id(new_id);
  new_id++;
}

void ReturnRaw::visit(BDDVisitor &visitor) const { visitor.visit(this); }

std::string ReturnRaw::dump(bool one_liner) const {
  std::stringstream ss;
  ss << id << ":";
  ss << "return raw";
  return ss.str();
}

} // namespace BDD