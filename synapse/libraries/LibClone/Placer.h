#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Nodes/Node.h>

#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibCore::Symbols;

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibBDD::Branch;

struct target_roots_t {
  bdd_node_ids_t port_roots;
  bdd_node_ids_t target_roots;
};

class NetworkPartitioner {
private:
  std::shared_ptr<const BDD> bdd;
  const PhysicalNetwork &phys_net;

public:
  NetworkPartitioner(const BDD &_bdd, const PhysicalNetwork &_phys_net);

  NetworkPartitioner(const NetworkPartitioner &)            = delete;
  NetworkPartitioner &operator=(const NetworkPartitioner &) = delete;

  const BDD *get_bdd() const { return bdd.get(); }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> process();

  void debug() const;
};
} // namespace LibClone
