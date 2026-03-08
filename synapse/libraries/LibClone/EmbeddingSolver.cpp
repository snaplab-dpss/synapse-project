#include <LibClone/EmbeddingSolver.h>

//#include <gurobi_c++.h>

namespace LibClone {

EmbeddingSolver::EmbeddingSolver(const PhysicalNetwork &_phys_net) : phys_net(_phys_net) {}

void EmbeddingSolver::set_time_limit(double seconds) { time_limit = seconds; }

void EmbeddingSolver::set_verbose(bool verbose_flag) { verbose = verbose_flag; }

void EmbeddingSolution::debug() const {
  std::cerr << "========== Embedding Solution ==========\n";
  std::cerr << "Feasible: " << (feasible ? "Yes" : "No") << "\n";
  if (feasible) {
    std::cerr << "Objective Value: " << objective_value << "\n";
    std::cerr << "Active Devices: ";
    for (const auto &[device, active] : device_active) {
      if (active)
        std::cerr << device << " ";
    }
    std::cerr << "\n";
    std::cerr << "Node Assignments:\n";
    for (const auto &[node, device] : node_to_device) {
      std::cerr << "  MetaNode " << node << " -> Device " << device << "\n";
    }
  }
  std::cerr << "========================================\n";
}

} // namespace LibClone
