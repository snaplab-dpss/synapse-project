#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Nodes/Node.h>

#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::Branch;

class Placer {
private:
  std::shared_ptr<const BDD> bdd;
  const PhysicalNetwork &phys_net;

public:
  Placer(const BDD &_bdd, const PhysicalNetwork &_phys_net);

  Placer(const Placer &)            = delete;
  Placer &operator=(const Placer &) = delete;

  const BDD *get_bdd() const { return bdd.get(); }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  std::unique_ptr<BDD> add_send_to_device_nodes();

private:
  std::optional<BDDNode *> create_send_to_device_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current, bdd_node_id_t next_node);
  std::optional<BDDNode *> create_parse_header_cpu_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current);
  void handle_branch_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id);
  void handle_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id);
};
} // namespace LibClone
