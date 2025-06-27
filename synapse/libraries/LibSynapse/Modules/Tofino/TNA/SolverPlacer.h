#pragma once

#include <LibSynapse/Modules/Tofino/TNA/Pipeline.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibCore/Types.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {
namespace SolverPlacer {

PlacementResult find_placements(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps);

}
} // namespace Tofino
} // namespace LibSynapse