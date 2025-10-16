#include <LibClone/Device.h>
#include <LibClone/Placer.h>
#include <LibClone/InfrastructureNode.h>

#include <LibCore/Debug.h>

#include <LibBDD/CallPath.h>
#include <LibBDD/Nodes/Node.h>

#include <LibSynapse/Target.h>

#include <cassert>
#include <queue>
#include <vector>
#include <optional>

namespace LibClone {

using LibCore::Symbols;

using LibBDD::arg_t;
using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::BDDNodeType;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibSynapse::InstanceId;
using LibSynapse::TargetType;

using LibClone::ComponentId;
using LibClone::DeviceId;
using LibClone::Port;

namespace {
BDD *setup_bdd(const BDD &bdd) {
  BDD *new_bdd = new BDD(bdd);
  return new_bdd;
}
} // namespace

Placer::Placer(const BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(setup_bdd(_bdd)), phys_net(_phys_net) {}

std::optional<BDDNode *> Placer::create_send_to_device_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_node_id) {

  const BDDNode *current_node = new_bdd->get_node_by_id(current_id);
  assert(current_node && "BDDNode not found");

  const BDDNode *next_node = new_bdd->get_node_by_id(next_node_id);
  assert(next_node && "BDDNode not found");

  if (phys_net.get_placement(current_id) == phys_net.get_placement(next_node_id)) {
    std::cerr << "No send_to_device needed: current and next are on the same device.\n";
    return std::nullopt;
  }

  InfrastructureNodeId current_instance_id = phys_net.get_placement(current_id).instance_id;
  InfrastructureNodeId next_instance_id    = phys_net.get_placement(next_node_id).instance_id;

  Port port         = phys_net.get_forwarding_port(current_instance_id, next_instance_id);
  TargetType target = phys_net.get_placement(next_instance_id);

  klee::ref<klee::ConstantExpr> port_expr   = klee::ConstantExpr::alloc(port, sizeof(port) * 8);
  klee::ref<klee::ConstantExpr> target_expr = klee::ConstantExpr::alloc(target.instance_id, sizeof(InstanceId) * 8);

  const arg_t port_arg{
      .expr        = port_expr,
      .fn_ptr_name = {},
      .in          = {},
      .out         = {},
      .meta        = {},
  };

  const arg_t target_arg{
      .expr        = target_expr,
      .fn_ptr_name = {},
      .in          = {},
      .out         = {},
      .meta        = {},

  };

  const call_t call{
      .function_name = "send_to_device",
      .extra_vars    = {},
      .args          = {{"outgoing_port", port_arg}, {"next_target", target_arg}},
      .ret           = {},
  };

  Symbols symbols;
  Call *s2d = new_bdd->create_new_call(current_node, call, symbols);

  return s2d;
}

void Placer::handle_branch_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id) {
  // Create send_to_device node for the "true" branch
  std::optional<BDDNode *> s2d_true = create_send_to_device_node(new_bdd, branch_id, on_true_id);
  if (s2d_true.has_value()) {
    new_bdd->add_cloned_non_branches(branch_id, {s2d_true.value()});
  }

  // Create send_to_device node for the "false" branch
  std::optional<BDDNode *> s2d_false = create_send_to_device_node(new_bdd, branch_id, on_false_id);
  if (s2d_false.has_value()) {
    new_bdd->add_cloned_non_branches(branch_id, {s2d_false.value()});
  }
}

void Placer::handle_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id) {
  // Create send_to_device node
  std::optional<BDDNode *> s2d = create_send_to_device_node(new_bdd, current_id, next_id);
  if (s2d.has_value()) {
    new_bdd->add_cloned_non_branches(current_id, {s2d.value()});
  }
}

std::unique_ptr<BDD> Placer::add_send_to_device_nodes() {
  std::cerr << "==========================================\n";

  const BDD *old_bdd           = get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *root = new_bdd->get_root();

  std::queue<const BDDNode *> queue;

  queue.push(root);

  bool in_root = true;

  while (!queue.empty()) {
    const BDDNode *current = queue.front();
    std::cerr << current->dump(true) << "\n";
    queue.pop();

    if (current->get_type() == BDDNodeType::Branch) {
      const Branch *branch    = dynamic_cast<const Branch *>(current);
      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();
      assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");

      if (!in_root) {
        handle_branch_node(new_bdd, branch->get_id(), on_true->get_id(), on_false->get_id());
      }

      queue.push(on_true);
      queue.push(on_false);
    } else {
      if (current->get_next()) {
        const BDDNode *next = current->get_next();

        if (!in_root) {
          handle_node(new_bdd, current->get_id(), next->get_id());
        }
        queue.push(next);
      }
    }
    in_root = false;
  }
  return new_bdd;
}

} // namespace LibClone
