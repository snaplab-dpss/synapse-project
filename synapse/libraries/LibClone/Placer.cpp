#include <LibClone/Placer.h>
#include <LibClone/InfrastructureNode.h>

#include <LibCore/Debug.h>

#include <LibBDD/CallPath.h>

#include <LibSynapse/Target.h>

#include <cassert>
#include <queue>
#include <vector>
#include <optional>

namespace LibClone {

using LibCore::Symbols;

using LibBDD::arg_t;
using LibBDD::BDD;
using LibBDD::BDDNode;
using LibBDD::BDDNodeType;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibSynapse::TargetType;

using LibClone::ComponentId;
using LibClone::Port;

std::optional<BDDNode *> NetworkPlacer::create_send_to_device_node(BDDNode *current, BDDNode *next_node) {

  ComponentId current_node_id = current->get_id();
  ComponentId next_node_id    = next_node->get_id();

  if (phys_net.get_placement(current_node_id) == phys_net.get_placement(next_node_id)) {
    return std::nullopt;
  }

  InfrastructureNodeId current_instance_id = phys_net.get_placement(current_node_id).instance_id;
  InfrastructureNodeId next_instance_id    = phys_net.get_placement(next_node_id).instance_id;

  Port port = phys_net.get_forwarding_port(current_instance_id, next_instance_id);

  klee::ref<klee::ConstantExpr> port_expr = klee::ConstantExpr::alloc(port, sizeof(port) * 8);

  const arg_t port_arg{
      .expr        = port_expr,
      .fn_ptr_name = {},
      .in          = {},
      .out         = {},
      .meta        = {},
  };

  const call_t call{
      .function_name = "send_to_device",
      .extra_vars    = {},
      .args          = {{"outgoing_port", port_arg}},
      .ret           = {},
  };

  Symbols symbols;
  Call *s2d = bdd.create_new_call(current, call, symbols);

  return s2d;
}

void NetworkPlacer::handle_branch_node(Branch *branch, BDDNode *on_true, BDDNode *on_false) {
  std::optional<BDDNode *> s2d_true = create_send_to_device_node(branch, on_true);

  if (s2d_true.has_value()) {
    branch->set_on_true(s2d_true.value());
    s2d_true.value()->set_prev(branch);

    s2d_true.value()->set_next(on_true);
    on_true->set_prev(s2d_true.value());
    std::cerr << s2d_true.value()->dump(true) << "\n";
  }

  std::optional<BDDNode *> s2d = create_send_to_device_node(branch, on_false);

  if (s2d.has_value()) {
    branch->set_on_false(s2d.value());
    s2d.value()->set_prev(branch);

    s2d.value()->set_next(on_false);
    on_false->set_prev(s2d.value());
    std::cerr << s2d.value()->dump(true) << "\n";
  }
}

void NetworkPlacer::handle_node(BDDNode *current, BDDNode *next) {
  std::optional<BDDNode *> s2d = create_send_to_device_node(current, next);
  if (s2d.has_value()) {
    current->set_next(s2d.value());
    s2d.value()->set_prev(current);

    s2d.value()->set_next(next);
    next->set_prev(s2d.value());
    std::cerr << s2d.value()->dump(true) << "\n";
  }
}

const BDD NetworkPlacer::add_send_to_device_nodes() {
  std::cerr << "==========================================\n";

  BDDNode *root = bdd.get_mutable_root();

  std::queue<BDDNode *> queue;

  queue.push(root);

  while (!queue.empty()) {
    BDDNode *current = queue.front();
    std::cerr << current->dump(true) << "\n";
    queue.pop();

    if (current->get_type() == BDDNodeType::Branch) {
      Branch *branch    = dynamic_cast<Branch *>(current);
      BDDNode *on_true  = branch->get_mutable_on_true();
      BDDNode *on_false = branch->get_mutable_on_false();

      handle_branch_node(branch, on_true, on_false);

      queue.push(on_true);
      queue.push(on_false);
    } else {
      if (current->get_next()) {
        BDDNode *next = current->get_mutable_next();
        handle_node(current, next);
        queue.push(next);
      }
    }
  }
  return bdd;
}

} // namespace LibClone
