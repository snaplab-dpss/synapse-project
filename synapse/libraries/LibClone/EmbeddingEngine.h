#pragma once

#include <LibCore/Types.h>

#include <LibClone/MetaNode.h>
#include <LibClone/EmbeddingProfiler.h>
#include <LibClone/EmbeddingSolver.h>
#include <unordered_map>
#include <vector>

namespace LibClone {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;

using LibSynapse::TargetType;

using LibClone::BDDInfo;
using LibClone::CostMatrix;
using LibClone::Device;
using LibClone::DeviceId;
using LibClone::EmbeddingCosts;
using LibClone::InfrastructureInfo;

class EmbeddingEngine {
private:
  const BDD &bdd;
  const PhysicalNetwork &phys_net;

  EmbeddingProfilers profilers;
  EmbeddingSolver solver;

public:
  EmbeddingEngine(const BDD &_bdd, const PhysicalNetwork &_phys_net);

  EmbeddingEngine(const EmbeddingEngine &)            = delete;
  EmbeddingEngine &operator=(const EmbeddingEngine &) = delete;

  const BDD &get_bdd() const { return bdd; }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  BDDInfo get_bdd_info() const;
  InfrastructureInfo get_infrastructure_info() const;
  std::unordered_map<TargetType, EmbeddingCosts> compute_costs() const;

  EmbeddingSolution solve(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info, const std::unordered_map<TargetType, EmbeddingCosts> &costs);
  const EmbeddingSolution solve();

  void debug(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const;
};

} // namespace LibClone
