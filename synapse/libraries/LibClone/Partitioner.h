#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Nodes/Node.h>

#include <LibClone/EmbeddingSolver.h>
#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibCore::Symbols;

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibBDD::Branch;

using LibClone::EmbeddingSolution;

struct target_roots_t {
  bdd_node_ids_t port_roots;
  bdd_node_ids_t target_roots;
};

class NetworkPartitioner {
private:
  const BDD &bdd;
  const PhysicalNetwork &phys_net;
  const EmbeddingSolution &solution;

public:
  NetworkPartitioner(const BDD &_bdd, const PhysicalNetwork &_phys_net, const EmbeddingSolution &_solution);

  NetworkPartitioner(const NetworkPartitioner &)            = delete;
  NetworkPartitioner &operator=(const NetworkPartitioner &) = delete;

  const BDD &get_bdd() const { return bdd; }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  std::unordered_map<DeviceId, std::unique_ptr<const BDD>> partition();

  void debug() const;
};
} // namespace LibClone
