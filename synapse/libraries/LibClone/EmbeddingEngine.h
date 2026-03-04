#pragma once

#include <LibClone/MetaNode.h>

#include <unordered_map>

namespace LibClone {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;

using LibClone::ComponentId;
using LibClone::DeviceId;

class EmbeddingEngine {
private:
  BDD bdd;
  const PhysicalNetwork &phys_net;

  std::unordered_map<MetaNodeId, std::shared_ptr<MetaNode>> meta_nodes;

  // EmbeddingSolver solver;

public:
  EmbeddingEngine(const BDD &_bdd, const PhysicalNetwork &_phys_net);
  EmbeddingEngine(const EmbeddingEngine &)            = delete;
  EmbeddingEngine &operator=(const EmbeddingEngine &) = delete;

  const BDD *get_bdd() const { return &bdd; }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  void pre_process();
  void debug() const;
};
} // namespace LibClone