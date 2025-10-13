#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Nodes/Node.h>

#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibBDD::BDD;
using LibBDD::BDDNode;
using LibBDD::Branch;

class Placer {
private:
  BDD &bdd;
  const PhysicalNetwork &phys_net;

public:
  Placer(BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(_bdd), phys_net(_phys_net) {}

  Placer(const Placer &)            = delete;
  Placer &operator=(const Placer &) = delete;

  const BDD &get_bdd() const { return bdd; }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  const BDD add_send_to_device_nodes();

private:
  std::optional<LibBDD::BDDNode *> create_send_to_device_node(BDDNode *current, BDDNode *next);
  void handle_branch_node(Branch *branch, BDDNode *on_true, BDDNode *on_false);
  void handle_node(BDDNode *current, BDDNode *next);
};
} // namespace LibClone
