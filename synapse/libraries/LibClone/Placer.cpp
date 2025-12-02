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

Placer::Placer(const BDD &_bdd, PhysicalNetwork &_phys_net) : bdd(setup_bdd(_bdd)), phys_net(_phys_net) {}

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

  // std::cerr << "CURRENT BDDNODE: " << current_node->dump(true) << "\n";
  // std::cerr << "NEXT BDDNODE: " << next_node->dump(true) << "\n";
  //
  // std::cerr << "CURRENT: " << current_instance_id << "\n";
  // std::cerr << "NEXT: " << next_instance_id << "\n";

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

  std::cerr << s2d->dump(true) << "\n";

  const InfrastructureNode *current_infrastructure_node = phys_net.get_node(current_instance_id).get();

  if (current_infrastructure_node->has_link(port)) {
    const InfrastructureNode *next_infrastructure_node = current_infrastructure_node->get_link(port).second;
    const Device *next_device                          = phys_net.get_device(next_infrastructure_node->get_id()).get();
    phys_net.add_placement(s2d->get_id(), next_device->get_target());
  } else {
    panic("No link found in physical network for send_to_device");
  }

  return s2d;
}

std::optional<BDDNode *> Placer::create_parse_header_cpu_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id) {

  const BDDNode *current_node = new_bdd->get_node_by_id(current_id);
  assert(current_node && "BDDNode not found");

  const call_t call{
      .function_name = "packet_parse_header_cpu",
      .extra_vars    = {},
      .args          = {},
      .ret           = {},
  };

  Symbols symbols;
  Call *parse_cpu = new_bdd->create_new_call(current_node, call, symbols);

  return parse_cpu;
}

std::vector<const BDDNode *> Placer::handle_branch_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id,
                                                        bdd_node_id_t on_false_id) {
  std::vector<const BDDNode *> inserted_nodes;
  std::optional<BDDNode *> s2d_true = create_send_to_device_node(new_bdd, branch_id, on_true_id);

  if (s2d_true.has_value()) {
    // std::cerr << s2d_true.value()->dump(true) << "\n";
    new_bdd->add_cloned_non_branches(on_true_id, {s2d_true.value()});
    inserted_nodes.push_back(new_bdd->get_node_by_id(branch_id)->get_next());
  } else {
    inserted_nodes.push_back(new_bdd->get_node_by_id(on_true_id));
  }

  std::optional<BDDNode *> s2d_false = create_send_to_device_node(new_bdd, branch_id, on_false_id);

  if (s2d_false.has_value()) {
    // std::cerr << s2d_false.value()->dump(true) << "\n";
    new_bdd->add_cloned_non_branches(on_false_id, {s2d_false.value()});
    inserted_nodes.push_back(new_bdd->get_node_by_id(branch_id)->get_next());
  } else {
    inserted_nodes.push_back(new_bdd->get_node_by_id(on_false_id));
  }

  return inserted_nodes;
}

std::vector<const BDDNode *> Placer::handle_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id) {
  std::vector<const BDDNode *> inserted_nodes;
  std::optional<BDDNode *> s2d = create_send_to_device_node(new_bdd, current_id, next_id);
  if (s2d.has_value()) {
    new_bdd->add_cloned_non_branches(next_id, {s2d.value()});
    inserted_nodes.push_back(new_bdd->get_node_by_id(current_id)->get_next());
  } else {
    inserted_nodes.push_back(new_bdd->get_node_by_id(next_id));
  }

  return inserted_nodes;
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
    queue.pop();

    std::vector<const BDDNode *> to_visit;

    if (current->get_type() == BDDNodeType::Branch) {
      const Branch *branch    = dynamic_cast<const Branch *>(current);
      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();
      assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");

      if (!in_root) {
        to_visit = handle_branch_node(new_bdd, branch->get_id(), on_true->get_id(), on_false->get_id());
      }

    } else {
      if (current->get_next()) {
        const BDDNode *next = current->get_next();

        to_visit = handle_node(new_bdd, current->get_id(), next->get_id());
      }
    }

    for (const BDDNode *node : to_visit) {
      queue.push(node);
    }

    in_root = false;
  }
  return new_bdd;
}

} // namespace LibClone
