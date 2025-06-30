#pragma once

#include <LibSynapse/Modules/Tofino/TNA/Pipeline.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibCore/Types.h>

#include <ostream>
#include <unordered_set>

namespace LibSynapse {
namespace Tofino {
namespace SolverPlacer {

enum class ActiveSolver { Z3, Gurobi };

inline std::ostream &operator<<(std::ostream &os, const ActiveSolver &solver) {
  switch (solver) {
  case ActiveSolver::Z3:
    os << "Z3";
    break;
  case ActiveSolver::Gurobi:
    os << "Gurobi";
    break;
  }
  return os;
}

ActiveSolver get_active_solver();
PlacementResult find_placements(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_gurobi(const Pipeline &pipeline, const DS *target_ds, const std::unordered_set<DS_ID> &deps);

} // namespace SolverPlacer
} // namespace Tofino
} // namespace LibSynapse