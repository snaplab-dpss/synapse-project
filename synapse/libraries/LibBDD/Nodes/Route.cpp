#include <LibBDD/Nodes/Route.h>
#include <LibBDD/Nodes/NodeManager.h>

namespace LibBDD {

Node *Route::clone(NodeManager &manager, bool recursive) const {
  Route *clone;

  if (recursive && next) {
    Node *next_clone = next->clone(manager, true);
    clone            = new Route(id, next_clone, nullptr, constraints, symbol_manager, operation, dst_device);
    next_clone->set_prev(clone);
  } else {
    clone = new Route(id, constraints, symbol_manager, operation, dst_device);
  }

  manager.add_node(clone);
  return clone;
}

std::string Route::dump(bool one_liner, bool id_name_only) const {
  std::stringstream ss;
  ss << id << ":";

  switch (operation) {
  case RouteOp::Forward:
    ss << "FORWARD";
    break;
  case RouteOp::Drop:
    ss << "DROP";
    break;
  case RouteOp::Broadcast:
    ss << "BROADCAST";
    break;
  }

  return ss.str();
}

} // namespace LibBDD