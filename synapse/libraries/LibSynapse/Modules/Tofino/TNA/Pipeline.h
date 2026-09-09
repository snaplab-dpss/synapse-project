#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibCore/Cow.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>
#include <LibCore/Types.h>

#include <cstdlib>
#include <unordered_set>
#include <vector>

namespace LibSynapse {
namespace Tofino {

enum class PlacementStatus {
  Success,
  TooLarge,
  TooManyKeys,
  XBarConsumptionExceedsLimit,
  NoAvailableStage,
  InconsistentPlacement,
  SelfDependence,
  NotEnoughDigests,
  UnmetDependencies,
  MultipleReasons,
  Unsat,
  Unknown,
};

inline std::ostream &operator<<(std::ostream &os, const PlacementStatus &status) {
  switch (status) {
  case PlacementStatus::Success:
    os << "Success";
    break;
  case PlacementStatus::TooLarge:
    os << "Too large";
    break;
  case PlacementStatus::TooManyKeys:
    os << "Too many keys";
    break;
  case PlacementStatus::XBarConsumptionExceedsLimit:
    os << "Xbar consumption exceeds limit";
    break;
  case PlacementStatus::NoAvailableStage:
    os << "No available stage";
    break;
  case PlacementStatus::InconsistentPlacement:
    os << "Inconsistent placement";
    break;
  case PlacementStatus::SelfDependence:
    os << "Self dependence";
    break;
  case PlacementStatus::NotEnoughDigests:
    os << "Not enough digests";
    break;
  case PlacementStatus::UnmetDependencies:
    os << "Unmet dependencies";
    break;
  case PlacementStatus::MultipleReasons:
    os << "Multiple reasons";
    break;
  case PlacementStatus::Unsat:
    os << "Unsat";
    break;
  case PlacementStatus::Unknown:
    os << "Unknown";
    break;
  }
  return os;
}

inline std::string placement_status_to_string(const PlacementStatus &status) {
  std::stringstream ss;
  ss << status;
  return ss.str();
}

struct Stage {
  int stage_id;
  bits_t available_sram;
  bits_t available_tcam;
  bits_t available_map_ram;
  bits_t available_exact_match_xbar;
  int available_logical_ids;
  int available_hash_dist_units;
  std::unordered_set<DS_ID> data_structures;
};

struct PipelineResources {
  std::vector<Stage> stages;
  u8 used_digests;

  // Computations placed in each gress, for the whole program. Neither is ever reset: bf-p4c lays
  // a gress out as a whole, so what a recirculated packet computes on its later laps still has to
  // sit alongside the first lap's work, and the same holds for the egress across passes. Crossing
  // moves the charge to the other gress rather than clearing anything, so the two budgets
  // together are all a solution ever gets. (Two earlier attempts reset per pass and per crossing;
  // both modelled a constraint the hardware does not have.)
  int used_compute_ops_ingress;
  int used_compute_ops_egress;
  bool building_egress;
  int pass_compute_ops; // Debug scaffolding only, see pass_compute_op_fits().

  PipelineResources(const tna_properties_t &properties);
  PipelineResources(const PipelineResources &other);
  PipelineResources &operator=(const PipelineResources &other) = default;
};

// A dependency set is fixed once requested; requests share it across the many copies of a
// pipeline (one per speculation), so copying a pipeline costs O(requests), not O(requests^2).
struct PlacementRequest {
  DS_ID ds;
  std::shared_ptr<const std::unordered_set<DS_ID>> deps;
};

struct PlacementResult {
  PlacementStatus status;
  std::optional<PipelineResources> resources;

  PlacementResult();
  PlacementResult(PlacementStatus _status);
  PlacementResult(const PipelineResources &_resources);
};

struct Pipeline {
  const tna_properties_t &properties;
  const DataStructures &data_structures;

  // Both change only when something is placed, and a pipeline is copied far more often than
  // that (once per speculation): copies share them until then.
  LibCore::Cow<PipelineResources> resources;
  LibCore::Cow<std::vector<PlacementRequest>> placement_requests;

  Pipeline(const tna_properties_t &_properties, const DataStructures &_data_structures);
  Pipeline(const Pipeline &other, const DataStructures &_data_structures);

  u8 get_used_stages() const {
    return std::count_if(resources->stages.begin(), resources->stages.end(), [](const Stage &stage) { return !stage.data_structures.empty(); });
  }

  bits_t get_memory_usage() const {
    bits_t used_memory = 0;
    for (const Stage &stage : resources->stages) {
      used_memory += properties.sram_per_stage - stage.available_sram;
      used_memory += properties.tcam_per_stage - stage.available_tcam;
      used_memory += properties.map_ram_per_stage - stage.available_map_ram;
    }
    return used_memory;
  }

  u8 get_used_digests() const { return resources->used_digests; }

  int get_used_compute_ops() const {
    return resources->building_egress ? resources->used_compute_ops_egress : resources->used_compute_ops_ingress;
  }
  bool compute_op_fits() const { return get_used_compute_ops() < properties.max_compute_ops_per_gress; }
  void charge_compute_op() {
    PipelineResources &r = resources.mutate();
    (r.building_egress ? r.used_compute_ops_egress : r.used_compute_ops_ingress)++;
  }
  void cross_to_egress() {
    PipelineResources &r = resources.mutate();
    r.building_egress    = true;
    r.pass_compute_ops   = 0;
  }
  // A recirculated packet re-enters through the ingress, so later laps are charged there again.
  void back_to_ingress() {
    PipelineResources &r = resources.mutate();
    r.building_egress    = false;
    r.pass_compute_ops   = 0;
  }
  bool is_building_egress() const { return resources->building_egress; }

  // DEBUG SCAFFOLDING, off unless SYNAPSE_FORCE_EGRESS_AFTER is set. Caps what one *pass* puts in
  // a gress, which is not a real constraint -- a gress is laid out as a whole -- but it forces the
  // shape the hand-written solution has (a little in ingress, the bulk in egress, every lap) so we
  // can ask whether synapse can build a compiling program at all, separately from whether its
  // search would ever choose to.
  int get_pass_compute_ops() const { return resources->pass_compute_ops; }
  void charge_pass_compute_op() { resources.mutate().pass_compute_ops++; }
  bool pass_compute_op_fits() const {
    static const char *limit = getenv("SYNAPSE_FORCE_EGRESS_AFTER");
    if (!limit || resources->building_egress) {
      return true;
    }
    return resources->pass_compute_ops < atoi(limit);
  }

  bool detect_changes_to_already_placed_data_structure(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  int get_soonest_stage_satisfying_all_dependencies(const std::unordered_set<DS_ID> &deps) const;

  int get_placed_stage(DS_ID ds_id) const;
  bool already_requested(DS_ID ds_id) const;
  bool already_placed(DS_ID ds_id) const;

  void place(const DS *ds, const std::unordered_set<DS_ID> &deps);
  // The stage a new compute action would take under the simple placer's rules, or -1.
  int find_stage_for_compute_action(const ComputeAction *action, const std::unordered_set<DS_ID> &deps) const;
  // An op appended to a placed compute action: charges its extra hash-distribution units in
  // the action's stage and extends the request's dependencies (for later re-placements).
  void append_to_compute_action(DS_ID action_id, int extra_hash_dist_units, const std::unordered_set<DS_ID> &extra_deps);
  PlacementStatus can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  // Simple placer only (no ILP fallback): cheap and conservative, for speculation.
  PlacementStatus can_place_fast(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  PlacementResult find_placements(const DS *ds, const std::unordered_set<DS_ID> &deps) const;

  void debug() const;
  void dump(std::ostream &os) const;
};

} // namespace Tofino
} // namespace LibSynapse