#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibCore/Cow.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>
#include <LibCore/Types.h>

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

  // Bits of PHV the current pass has committed in the gress it is currently building. Unlike
  // everything else here it is not cumulative over the whole program: a recirculation re-parses
  // and an egress crossing moves to the other gress's own PHV partition, so both reset it.
  //
  // The charge is deliberately crude -- every computed value costs its width and nothing is ever
  // given back when it dies -- because its only job is to make a gress fill up. See the note on
  // phv_bits_per_pass_per_gress in the target config.
  bits_t used_phv_bits;

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

  bits_t get_used_phv_bits() const { return resources->used_phv_bits; }
  bool phv_fits(bits_t extra) const { return resources->used_phv_bits + extra <= properties.phv_bits_per_pass_per_gress; }
  void charge_phv(bits_t bits) { resources.mutate().used_phv_bits += bits; }
  void reset_phv() { resources.mutate().used_phv_bits = 0; }

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