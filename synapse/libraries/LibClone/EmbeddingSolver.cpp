#include "LibClone/Device.h"
#include "LibCore/Debug.h"
#include <LibClone/EmbeddingSolver.h>

#include <gurobi_c++.h>
#include <iomanip>
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>

namespace LibClone {

using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;

using LibClone::MetaNodeType;

using VariableSet    = std::unordered_map<bdd_node_id_t, std::unordered_map<DeviceId, GRBVar>>;
using HopVariableSet = std::map<std::pair<bdd_node_id_t, bdd_node_id_t>, GRBVar>;

namespace {

NodeClassification classify_nodes(const MetaNodes &meta_nodes) {
  NodeClassification result;

  for (const MetaNode *meta_node : meta_nodes.get_all()) {
    const bdd_node_ids_t &node_ids = meta_node->get_nodes();

    switch (meta_node->get_type()) {
    case MetaNodeType::GLOBAL_PORT:
      result.N_port.insert(node_ids.begin(), node_ids.end());
      break;
    case MetaNodeType::PROCESS:
    case MetaNodeType::DATA_STRUCTURE:
      result.N_free.insert(node_ids.begin(), node_ids.end());
      break;
    }
  }

  return result;
}

VariableSet create_variables(GRBModel &model, const bdd_node_ids_t &nodes, const MetaNodes &meta_nodes, const std::vector<DeviceId> &device_ids) {
  VariableSet x;

  for (const bdd_node_id_t &node_id : nodes) {
    assert_or_panic(meta_nodes.has_node(node_id), "BDDNode not in MetaNodes");
    assert_or_panic(x.find(node_id) == x.end(), "BDDNode should not be in problem modelling yet");

    x[node_id];

    const MetaNode *meta_node = meta_nodes.find_by_node(node_id);
    for (const DeviceId &dev_id : device_ids) {
      assert_or_panic(x.at(node_id).find(dev_id) == x.at(node_id).end(), "Device should not be in problem modelling yet");

      std::stringstream ss;
      ss << "x_fixed_" << node_id << "_" << dev_id;

      if (meta_node->is_fixed()) {
        const DeviceId &fixed_device = meta_node->get_assigned_device();
        double fixed_val             = (dev_id == fixed_device) ? 1.0 : 0.0;
        x[node_id][dev_id]           = model.addVar(fixed_val, fixed_val, 0.0, GRB_CONTINUOUS, ss.str());
      } else {
        x[node_id][dev_id] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());
      }
    }
  }

  return x;
}

HopVariableSet create_hop_variables(GRBModel &model, const std::vector<std::pair<bdd_node_id_t, bdd_node_id_t>> &links, const CostMatrix &cost_matrix,
                                    u16 h_max) {
  HopVariableSet h;

  for (const std::pair<bdd_node_id_t, bdd_node_id_t> &link : links) {
    assert_or_panic(h.find(link) == h.end(), "Link shouldn't be in the hop variables yet");

    std::stringstream ss;
    ss << "h_" << link.first << "_" << link.second;

    h[link] = model.addVar(0.0, h_max, 0.0, GRB_CONTINUOUS, ss.str());
  }

  return h;
}

void add_single_placement_constraints(GRBModel &model, const VariableSet &x, const bdd_node_ids_t &nodes, const std::vector<DeviceId> &device_ids) {
  for (const bdd_node_id_t &node_id : nodes) {
    assert_or_panic(x.find(node_id) != x.end(), "BDDNode not in problem modelling");
    GRBLinExpr sum = 0;
    for (const DeviceId &dev_id : device_ids) {
      assert_or_panic(x.at(node_id).find(dev_id) != x.at(node_id).end(), "Device not in problem modelling");
      sum += x.at(node_id).at(dev_id);
    }
    model.addConstr(sum == 1.0, "single_placement_" + std::to_string(node_id));
  }
}

void add_meta_node_co_location_constraints(GRBModel &model, const VariableSet &x, const MetaNodes &meta_nodes,
                                           const std::vector<DeviceId> &device_ids) {
  for (const MetaNode *meta_node : meta_nodes.get_all()) {
    const bdd_node_ids_t &meta_nodes_set = meta_node->get_nodes();
    if (meta_nodes_set.size() == 1)
      continue;

    bdd_node_id_t rep = *meta_nodes_set.begin();
    assert_or_panic(x.find(rep) != x.end(), "BDDNode not in problem modelling");

    for (const bdd_node_id_t &node_id : meta_nodes_set) {
      assert_or_panic(x.find(node_id) != x.end(), "BDDNode not in problem modelling");
      if (node_id == rep) {
        continue;
      }
      for (const DeviceId &dev_id : device_ids) {
        assert_or_panic(x.at(node_id).find(dev_id) != x.at(node_id).end(), "Device not in problem modelling");
        assert_or_panic(x.at(rep).find(dev_id) != x.at(rep).end(), "Device not in problem modelling");
        model.addConstr(x.at(node_id).at(dev_id) == x.at(rep).at(dev_id), "MetaNode_colocate_" + std::to_string(node_id) + "_" + std::to_string(rep));
      }
    }
  }
}

void add_hop_free_to_free_constraint(GRBModel &model, const VariableSet &x, const GRBVar &hop_var, const bdd_node_id_t &src, const bdd_node_id_t &dst,
                                     const std::vector<DeviceId> &device_ids, const CostMatrix &cost_matrix) {

  std::map<std::pair<DeviceId, DeviceId>, GRBVar> w;

  for (DeviceId d1 : device_ids) {
    assert_or_panic(cost_matrix.find(d1) != cost_matrix.end(), "Device Not in Cost Matrix");
    for (DeviceId d2 : device_ids) {
      assert_or_panic(cost_matrix.at(d1).find(d2) != cost_matrix.at(d1).end(), "Device Not in Cost Matrix");

      std::stringstream ss;
      ss << "w_" << src << "_" << dst << "_" << d1 << "_" << d2;

      w[{d1, d2}] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());

      // Linearization constraints:
      // w <= x[src][d1]
      model.addConstr(w.at({d1, d2}) <= x.at(src).at(d1));
      // w <= x[dst][d2]
      model.addConstr(w.at({d1, d2}) <= x.at(dst).at(d2));
      // w >= x[src][d1] + x[dst][d2] - 1
      model.addConstr(w.at({d1, d2}) >= x.at(src).at(d1) + x.at(dst).at(d2) - 1);
    }
  }

  GRBLinExpr hop_expr = 0;
  for (DeviceId d1 : device_ids) {
    for (DeviceId d2 : device_ids) {

      u16 cost = cost_matrix.at(d1).at(d2);
      hop_expr += cost * w.at({d1, d2});
    }
  }
  model.addConstr(hop_var == hop_expr, "hop_free_free_node_" + std::to_string(src) + "_node_" + std::to_string(dst));
}

void add_hop_free_to_fixed_constraints(GRBModel &model, const VariableSet &x, const GRBVar &hop_var, const bdd_node_id_t &src,
                                       const DeviceId &dst_device, const std::vector<DeviceId> &device_ids, const CostMatrix &cost_matrix) {

  // C7: h[i][j] = sum_{d} cost[d][fixed_j] * x[i][d]
  GRBLinExpr hop_expr = 0;
  for (DeviceId d : device_ids) {
    assert_or_panic(cost_matrix.find(d) != cost_matrix.end(), "Device Not in Cost Matrix");
    assert_or_panic(cost_matrix.at(d).find(dst_device) != cost_matrix.at(d).end(), "Device Not in Cost Matrix");

    u16 cost = cost_matrix.at(d).at(dst_device);
    hop_expr += cost * x.at(src).at(d);
  }
  model.addConstr(hop_var == hop_expr, "hop_free_fixed_node_" + std::to_string(src) + "_device_" + std::to_string(dst_device));
}

void add_hop_fixed_to_free_constraints(GRBModel &model, const VariableSet &x, const GRBVar &hop_var, const DeviceId &src_device,
                                       const bdd_node_id_t &dst, const std::vector<DeviceId> &device_ids, const CostMatrix &cost_matrix) {

  // C8: h[i][j] = sum_{d} cost[fixed_i][d] * x[j][d]
  GRBLinExpr hop_expr = 0;
  for (DeviceId d : device_ids) {
    assert_or_panic(cost_matrix.find(src_device) != cost_matrix.end(), "Device Not in Cost Matrix");
    assert_or_panic(cost_matrix.at(src_device).find(d) != cost_matrix.at(src_device).end(), "Device Not in Cost Matrix");

    u16 cost = cost_matrix.at(src_device).at(d);
    hop_expr += cost * x.at(dst).at(d);
  }
  model.addConstr(hop_var == hop_expr, "hop_fixed_free_device_" + std::to_string(src_device) + "_node_" + std::to_string(dst));
}

void add_hop_fixed_to_fixed_constraints(GRBModel &model, const GRBVar &hop_var, const DeviceId &src_device, const DeviceId &dst_device,
                                        const CostMatrix &cost_matrix) {
  u16 cost = cost_matrix.at(src_device).at(dst_device);

  // C9: h[i][j] = delta(fixed_i, fixed_j)  (constant)
  model.addConstr(hop_var == cost, "hop_fixed_fixed_device_" + std::to_string(src_device) + "_device_" + std::to_string(dst_device));
}

void add_hop_count_constraints(GRBModel &model, const VariableSet &x, const HopVariableSet &h, const MetaNodes &meta_nodes,
                               const std::vector<std::pair<bdd_node_id_t, bdd_node_id_t>> &links, const std::vector<DeviceId> &device_ids,
                               const CostMatrix &cost_matrix) {

  for (const std::pair<bdd_node_id_t, bdd_node_id_t> &link : links) {
    assert_or_panic(h.find(link) != h.end(), "Link should be in HopVariableSet");

    GRBVar hop_var = h.at(link);

    const MetaNode *src_meta_node = meta_nodes.find_by_node(link.first);
    const MetaNode *dst_meta_node = meta_nodes.find_by_node(link.second);

    const bool src_fixed = src_meta_node->is_fixed();
    const bool dst_fixed = dst_meta_node->is_fixed();

    if (!src_fixed && !dst_fixed) {
      add_hop_free_to_free_constraint(model, x, hop_var, link.first, link.second, device_ids, cost_matrix);
    } else if (!src_fixed && dst_fixed) {
      add_hop_free_to_fixed_constraints(model, x, hop_var, link.first, dst_meta_node->get_assigned_device(), device_ids, cost_matrix);
    } else if (src_fixed && !dst_fixed) {
      add_hop_fixed_to_free_constraints(model, x, hop_var, src_meta_node->get_assigned_device(), link.second, device_ids, cost_matrix);
    } else if (src_fixed && dst_fixed) {
      add_hop_fixed_to_fixed_constraints(model, hop_var, src_meta_node->get_assigned_device(), dst_meta_node->get_assigned_device(), cost_matrix);
    } else {
      panic("Reached impossible condition state");
    }
  }
}

GRBLinExpr build_objective(const VariableSet &x, const HopVariableSet &h, const bdd_node_ids_t &nodes, const std::vector<DeviceId> &device_ids,
                           double alpha, double beta) {
  // Communication cost: sum of all hops
  GRBLinExpr comm_cost = 0;
  for (const auto &[edge, hop_var] : h) {
    comm_cost += hop_var;
  }

  // Embedding cost: sum of placement costs
  // For now, use uniform cost (1.0 per placement decision)
  // TODO: Replace with actual embedding cost f(i,d) if needed
  GRBLinExpr embed_cost = 0;
  for (bdd_node_id_t i : nodes) {
    for (DeviceId d : device_ids) {
      embed_cost += x.at(i).at(d);
    }
  }

  return alpha * embed_cost + beta * comm_cost;
}
} // namespace

void NodeClassification::debug() const {
  std::cerr << "========== Node Classification ==========\n";
  std::cerr << "N_free (need placement):  " << N_free.size() << " nodes\n";
  for (bdd_node_id_t id : N_free) {
    std::cerr << "  " << id << "\n";
  }

  std::cerr << "N_port (fixed):           " << N_port.size() << " nodes\n";
  for (bdd_node_id_t id : N_port) {
    std::cerr << "  " << id << "\n";
  }

  std::cerr << "=========================================\n";
}

EmbeddingSolver::EmbeddingSolver() = default;

void EmbeddingSolver::set_time_limit(u16 seconds) { time_limit = seconds; }

void EmbeddingSolver::set_verbose(bool verbose_flag) { verbose = verbose_flag; }

void EmbeddingSolver::set_alpha(double a) { alpha = a; }

void EmbeddingSolver::set_beta(double b) { beta = b; }

void EmbeddingSolver::set_h_max(u16 h) { h_max = h; }

EmbeddingSolution EmbeddingSolver::solve(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const {
  auto start_time = std::chrono::steady_clock::now();

  EmbeddingSolution solution;
  solution.status = SolutionStatus::Unknown;

  NodeClassification nodes = classify_nodes(bdd_info.meta_nodes);
  if (verbose) {
    nodes.debug();
  }

  try {
    // Gurobi environment
    GRBEnv env = GRBEnv(true);
    env.set(GRB_IntParam_OutputFlag, verbose ? 1 : 0);
    env.set(GRB_DoubleParam_TimeLimit, static_cast<double>(time_limit));
    env.start();

    GRBModel model = GRBModel(env);
    model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

    // Get device list
    std::vector<DeviceId> device_ids;
    for (const auto &[device_id, _] : infra_info.devices) {
      device_ids.push_back(device_id);
    }

    // Create Environment Vars
    VariableSet x    = create_variables(model, bdd_info.nodes, bdd_info.meta_nodes, device_ids);
    HopVariableSet h = create_hop_variables(model, bdd_info.links, infra_info.cost_matrix, h_max);

    // Constraints
    // Single placement constraint (C1)
    add_single_placement_constraints(model, x, bdd_info.nodes, device_ids);

    // Fixed Port Nodes (C2)
    // We already fix the value when adding the Vars. Adding a specific constraint is pointless

    // MetaNode co-location (C3)
    add_meta_node_co_location_constraints(model, x, bdd_info.meta_nodes, device_ids);

    // TODO: Capacity Constraints (C4, C5)

    // Hop Count Constraints (C6 - c9)
    add_hop_count_constraints(model, x, h, bdd_info.meta_nodes, bdd_info.links, device_ids, infra_info.cost_matrix);

    // Max Hop Count Constraint (C10)
    // h[link] is already bounded from 0 to h_max. No need to implement a specific constraint for it

    // Phase 4: Objective (placeholder - just minimize 0 for now)
    GRBLinExpr objective = build_objective(x, h, bdd_info.nodes, device_ids, alpha, beta);
    model.setObjective(objective);
    model.optimize();

    // Extract solution
    int status = model.get(GRB_IntAttr_Status);

    if (status == GRB_OPTIMAL) {
      solution.status = SolutionStatus::Optimal;
    } else if (status == GRB_TIME_LIMIT) {
      solution.status = SolutionStatus::Timeout;
    } else if (status == GRB_INFEASIBLE) {
      solution.status = SolutionStatus::Infeasible;
    }

    if (solution.is_valid()) {
      solution.total_cost = model.get(GRB_DoubleAttr_ObjVal);

      for (const bdd_node_id_t &i : bdd_info.nodes) {
        assert_or_panic(x.find(i) != x.end(), "BDDNode not in problem modelling");
        for (const DeviceId &d : device_ids) {
          assert_or_panic(x.at(i).find(d) != x.at(i).end(), "Device not in problem modelling");
          if (x.at(i).at(d).get(GRB_DoubleAttr_X) > 0.5) {
            solution.placement[i] = d;
            break;
          }
        }
      }
    }
  } catch (GRBException &e) {
    std::cerr << "Gurobi error: " << e.getMessage() << "\n";
    solution.status = SolutionStatus::Error;
  }

  auto end_time               = std::chrono::steady_clock::now();
  solution.solve_time_seconds = std::chrono::duration<double>(end_time - start_time).count();

  return solution;
}

DeviceId EmbeddingSolution::get_placement(const bdd_node_id_t &node_id) const {
  assert_or_panic(placement.find(node_id) != placement.end(), "Node %lu not found in Solution", node_id);
  return placement.at(node_id);
}

nlohmann::json EmbeddingSolution::to_json() const {

  nlohmann::json j;

  j["status"]             = solution_status_to_string(status);
  j["total_cost"]         = total_cost;
  j["solve_time_seconds"] = solve_time_seconds;
  j["is_valid"]           = is_valid();

  j["placements"] = nlohmann::json::array();
  for (const auto &[node_id, device_id] : placement) {
    nlohmann::json embedding;
    embedding["bdd_node_id"] = node_id;
    embedding["device_id"]   = device_id;
    j["placements"].push_back(embedding);
  }

  j["num_placements"] = placement.size();
  return j;
}

EmbeddingSolution EmbeddingSolution::from_json(const std::filesystem::path &fpath) {
  std::ifstream in(fpath.string());

  assert_or_panic(in, "Unable to open Solution file \"%s\"", fpath.c_str());

  nlohmann::json j;
  EmbeddingSolution sol;

  in >> j;
  // sol.status             = j["status"];
  sol.total_cost         = j["total_cost"];
  sol.solve_time_seconds = j["solve_time_seconds"];

  for (const nlohmann::json &embedding : j["placements"]) {
    sol.placement[embedding["bdd_node_id"]] = embedding["device_id"];
  }

  return sol;
}

void EmbeddingSolution::serealize(const std::filesystem::path &fpath) const {
  std::ofstream out(fpath);

  assert_or_panic(out, "Could not open file");
  assert_or_panic(out.is_open(), "File is not open");

  out << std::setw(2) << to_json() << std::endl;
}

void EmbeddingSolution::debug() const {
  std::cerr << "========== Embedding Solution ==========\n";
  std::cerr << "Status: " << solution_status_to_string(status) << "\n";
  std::cerr << "Total Cost: " << total_cost << "\n";
  std::cerr << "Solve Time: " << solve_time_seconds << "s\n";

  if (is_valid()) {
    std::cerr << "Placements (" << placement.size() << " nodes):\n";
    for (const auto &[node_id, device_id] : placement) {
      std::cerr << "  BDD Node " << node_id << " -> Device " << device_id << "\n";
    }
  }
  std::cerr << "========================================\n";
}

EmbeddingSolution::inspection_report_t EmbeddingSolution::inspect(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const {
  // Check if solution is valid first
  if (!is_valid()) {
    return {InspectionStatus::EmptySolution, "Solution is not valid (status: " + solution_status_to_string(status) + ")"};
  }

  // Check for empty placement
  if (placement.empty()) {
    return {InspectionStatus::EmptySolution, "Placement map is empty"};
  }

  // Check all BDD nodes have placements
  for (const bdd_node_id_t &node_id : bdd_info.nodes) {
    if (placement.find(node_id) == placement.end()) {
      return {InspectionStatus::MissingPlacement, "BDD node " + std::to_string(node_id) + " has no placement assignment"};
    }
  }

  // Check for duplicate placements (same node placed multiple times - shouldn't happen with map, but safety check)
  std::unordered_set<bdd_node_id_t> checked_nodes;
  for (const auto &[node_id, device_id] : placement) {
    if (!checked_nodes.insert(node_id).second) {
      return {InspectionStatus::DuplicateNodePlacement, "BDD node " + std::to_string(node_id) + " has duplicate placement entries"};
    }
  }

  // Check all placed nodes exist in BDD
  for (const auto &[node_id, device_id] : placement) {
    if (bdd_info.nodes.find(node_id) == bdd_info.nodes.end()) {
      return {InspectionStatus::NodeNotInBDD, "Placed node " + std::to_string(node_id) + " does not exist in BDD"};
    }
  }

  // Check all device assignments are valid
  for (const auto &[node_id, device_id] : placement) {
    if (device_id < 0) {
      return {InspectionStatus::InvalidDeviceId, "BDD node " + std::to_string(node_id) + " assigned invalid device ID " + std::to_string(device_id)};
    }

    if (infra_info.devices.find(device_id) == infra_info.devices.end()) {
      return {InspectionStatus::DeviceNotInInfrastructure,
              "BDD node " + std::to_string(node_id) + " assigned to unknown device " + std::to_string(device_id)};
    }
  }

  // Verify meta-node co-location constraints are satisfied
  for (const MetaNode *meta_node : bdd_info.meta_nodes.get_all()) {
    const bdd_node_ids_t &meta_nodes_set = meta_node->get_nodes();
    if (meta_nodes_set.size() <= 1)
      continue;

    bdd_node_id_t first_node = *meta_nodes_set.begin();
    auto first_it            = placement.find(first_node);
    if (first_it == placement.end())
      continue; // Already caught above

    DeviceId expected_device = first_it->second;

    for (const bdd_node_id_t &node_id : meta_nodes_set) {
      auto node_it = placement.find(node_id);
      if (node_it == placement.end())
        continue; // Already caught above

      if (node_it->second != expected_device) {
        return {InspectionStatus::InvalidDeviceId, "Meta-node co-location violated: node " + std::to_string(node_id) + " placed on device " +
                                                       std::to_string(node_it->second) + " but expected device " + std::to_string(expected_device) +
                                                       " (meta-node " + std::to_string(meta_node->get_id()) + ")"};
      }
    }
  }

  return {InspectionStatus::Ok, "Ok"};
}

void EmbeddingSolution::assert_inspection(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const {
  const inspection_report_t report = inspect(bdd_info, infra_info);
  assert_or_panic(report.status == InspectionStatus::Ok, "EmbeddingSolution inspection failed: %s", report.message.c_str());
  std::cerr << "SOLUTION REPORT STATUS: " << report.message.c_str() << "\n";
}

std::ostream &operator<<(std::ostream &os, const EmbeddingSolution::inspection_report_t &report) {
  os << "report{status=";
  switch (report.status) {
  case EmbeddingSolution::InspectionStatus::Ok:
    os << "Ok";
    break;
  case EmbeddingSolution::InspectionStatus::MissingPlacement:
    os << "MissingPlacement";
    break;
  case EmbeddingSolution::InspectionStatus::InvalidDeviceId:
    os << "InvalidDeviceId";
    break;
  case EmbeddingSolution::InspectionStatus::DeviceNotInInfrastructure:
    os << "DeviceNotInInfrastructure";
    break;
  case EmbeddingSolution::InspectionStatus::DuplicateNodePlacement:
    os << "DuplicateNodePlacement";
    break;
  case EmbeddingSolution::InspectionStatus::NodeNotInBDD:
    os << "NodeNotInBDD";
    break;
  case EmbeddingSolution::InspectionStatus::EmptySolution:
    os << "EmptySolution";
    break;
  }
  os << ", msg=" << report.message << "}";
  return os;
}

} // namespace LibClone
