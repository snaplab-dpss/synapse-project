#pragma once

#include <nlohmann/json.hpp>

#include <LibCore/Types.h>

#include <LibBDD/BDD.h>

#include <LibClone/MetaNode.h>
#include <LibClone/EmbeddingProfiler.h>
#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;

using LibClone::CostMatrix;
using LibClone::EmbeddingCost;
using LibClone::EmbeddingCosts;

struct BDDInfo {
  MetaNodes meta_nodes;
  bdd_node_ids_t nodes;
  std::vector<std::pair<bdd_node_id_t, bdd_node_id_t>> links;
};

struct InfrastructureInfo {
  std::unordered_map<DeviceId, const Device *> devices;
  CostMatrix cost_matrix;
};

struct NodeClassification {
  bdd_node_ids_t N_free;
  bdd_node_ids_t N_port;

  void debug() const;
};

enum class SolutionStatus {
  Unknown,
  Optimal,
  Feasible,   // Found a solution but not proven optimal
  Infeasible, // No solution exists
  Timeout,    // Hit time limit
  Error,
};

struct EmbeddingSolution {
  std::unordered_map<bdd_node_id_t, DeviceId> placement;

  double total_cost = 0.0;

  double solve_time_seconds = 0.0;

  SolutionStatus status = SolutionStatus::Unknown;

  bool is_valid() const { return status == SolutionStatus::Optimal || status == SolutionStatus::Feasible; }

  DeviceId get_placement(const bdd_node_id_t &node_id) const;

  nlohmann::json to_json() const;
  static EmbeddingSolution from_json(const std::filesystem::path &fpath);
  void serealize(const std::filesystem::path &fpath) const;
  void debug() const;

  enum class InspectionStatus {
    Ok,
    MissingPlacement,
    InvalidDeviceId,
    DeviceNotInInfrastructure,
    DuplicateNodePlacement,
    NodeNotInBDD,
    EmptySolution,
  };

  struct inspection_report_t {
    InspectionStatus status;
    std::string message;
  };

  [[nodiscard]] inspection_report_t inspect(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const;
  void assert_inspection(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const;
};

std::ostream &operator<<(std::ostream &os, const EmbeddingSolution::inspection_report_t &report);

inline const std::string &solution_status_to_string(SolutionStatus status) {
  static const std::string OPTIMAL_STR    = "OPTIMAL";
  static const std::string FEASIBLE_STR   = "FEASIBLE";
  static const std::string INFEASIBLE_STR = "INFEASIBLE";
  static const std::string TIMEOUT_STR    = "TIMEOUT";
  static const std::string UNKNOWN_STR    = "UNKNOWN";
  static const std::string ERROR_STR      = "ERROR";

  switch (status) {
  case SolutionStatus::Optimal:
    return OPTIMAL_STR;
  case SolutionStatus::Feasible:
    return FEASIBLE_STR;
  case SolutionStatus::Infeasible:
    return INFEASIBLE_STR;
  case SolutionStatus::Timeout:
    return TIMEOUT_STR;
  case SolutionStatus::Unknown:
    return UNKNOWN_STR;
  case SolutionStatus::Error:
    return ERROR_STR;
  }
  panic("SolutionStatus Unknown");
}

class EmbeddingSolver {
private:
  u16 time_limit = 3600;
  bool verbose   = false;
  double alpha   = 1.0;
  double beta    = 1.0;
  u16 h_max      = 20;

public:
  EmbeddingSolver();

  EmbeddingSolver(const EmbeddingSolver &)            = delete;
  EmbeddingSolver &operator=(const EmbeddingSolver &) = delete;

  void set_time_limit(u16 seconds);
  void set_verbose(bool verbose);
  void set_alpha(double a);
  void set_beta(double b);
  void set_h_max(u16 h);

  EmbeddingSolution solve(const BDDInfo &bdd_info, const InfrastructureInfo &infrastructure_info,
                          const std::unordered_map<TargetType, EmbeddingCosts> &f) const;

  void debug() const;
};

} // namespace LibClone
