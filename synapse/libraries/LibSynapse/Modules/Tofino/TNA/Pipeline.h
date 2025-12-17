#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
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
  std::unordered_set<DS_ID> data_structures;
};

struct PipelineResources {
  std::vector<Stage> stages;
  u8 used_digests;

  PipelineResources(const tna_properties_t &properties);
  PipelineResources(const PipelineResources &other);
  PipelineResources &operator=(const PipelineResources &other) = default;
};

struct PlacementRequest {
  DS_ID ds;
  std::unordered_set<DS_ID> deps;
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

  PipelineResources resources;
  std::vector<PlacementRequest> placement_requests;

  Pipeline(const tna_properties_t &_properties, const DataStructures &_data_structures);
  Pipeline(const Pipeline &other, const DataStructures &_data_structures);

  u8 get_used_stages() const {
    return std::count_if(resources.stages.begin(), resources.stages.end(), [](const Stage &stage) { return !stage.data_structures.empty(); });
  }

  u8 get_used_digests() const { return resources.used_digests; }

  bool detect_changes_to_already_placed_data_structure(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  int get_soonest_stage_satisfying_all_dependencies(const std::unordered_set<DS_ID> &deps) const;

  int get_placed_stage(DS_ID ds_id) const;
  bool already_requested(DS_ID ds_id) const;
  bool already_placed(DS_ID ds_id) const;

  void place(const DS *ds, const std::unordered_set<DS_ID> &deps);
  PlacementStatus can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  PlacementResult find_placements(const DS *ds, const std::unordered_set<DS_ID> &deps) const;

  void debug() const;
};

} // namespace Tofino
} // namespace LibSynapse