#pragma once

#include <LibSynapse/Modules/Tofino/TNA/Pipeline.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibCore/Types.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {
namespace SimplePlacer {

PlacementResult find_placements(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps);
// Every request placed again, in order, from an empty pipeline. `requests` stands in for the
// pipeline's own list: one of them may carry more dependencies than when it was placed (a
// composite with primitives placed since, a shared action another path reached later), and
// whatever depends on it follows it down.
PlacementResult replay_requests(const Pipeline &pipeline, const std::vector<PlacementRequest> &requests);

PlacementResult find_placements_table(const Pipeline &pipeline, const Table *table, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_reg(const Pipeline &pipeline, const Register *reg, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_meter(const Pipeline &pipeline, const Meter *meter, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_hash(const Pipeline &pipeline, const Hash *hash, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_digest(const Pipeline &pipeline, const Digest *digest, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_compute_action(const Pipeline &pipeline, const ComputeAction *action, const std::unordered_set<DS_ID> &deps);
PlacementResult find_placements_lpm(const Pipeline &pipeline, const LPM *lpm, const std::unordered_set<DS_ID> &deps);

} // namespace SimplePlacer
} // namespace Tofino
} // namespace LibSynapse