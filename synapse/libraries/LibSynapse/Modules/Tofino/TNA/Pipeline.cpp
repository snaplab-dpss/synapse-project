#include <LibSynapse/Modules/Tofino/TNA/Pipeline.h>
#include <LibSynapse/Modules/Tofino/TNA/SimplePlacer.h>
#include <LibSynapse/Modules/Tofino/TNA/SolverPlacer.h>
#include <LibCore/Debug.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::int2hr;

PlacementResult::PlacementResult() : status(PlacementStatus::Unknown) {}

PlacementResult::PlacementResult(PlacementStatus _status) : status(_status) {
  assert(status != PlacementStatus::Success && "PlacementResult should not be created with Success status");
}

PlacementResult::PlacementResult(const PipelineResources &_resources) : status(PlacementStatus::Success), resources(_resources) {}

PipelineResources::PipelineResources(const tna_properties_t &properties) {
  used_digests = 0;
  for (int stage_id = 0; stage_id < properties.stages; stage_id++) {
    const Stage s{
        .stage_id                   = stage_id,
        .available_sram             = properties.sram_per_stage,
        .available_tcam             = properties.tcam_per_stage,
        .available_map_ram          = properties.map_ram_per_stage,
        .available_exact_match_xbar = properties.exact_match_xbar_per_stage,
        .available_logical_ids      = properties.max_logical_sram_and_tcam_tables_per_stage,
        .data_structures            = {},
    };

    stages.push_back(s);
  }
}

PipelineResources::PipelineResources(const PipelineResources &other) : stages(other.stages), used_digests(other.used_digests) {}

Pipeline::Pipeline(const tna_properties_t &_properties, const DataStructures &_data_structures)
    : properties(_properties), data_structures(_data_structures), resources(_properties) {}

Pipeline::Pipeline(const Pipeline &other, const DataStructures &_data_structures)
    : properties(other.properties), data_structures(_data_structures), resources(other.resources), placement_requests(other.placement_requests) {}

void Pipeline::debug() const {
  std::stringstream ss;
  ss << "\n";
  ss << "====================== TNA Pipeline ======================\n";

  for (const Stage &stage : resources.stages) {
    if (stage.data_structures.empty()) {
      continue;
    }

    const double sram_usage    = 100.0 - (stage.available_sram * 100.0) / properties.sram_per_stage;
    const bits_t sram_consumed = properties.sram_per_stage - stage.available_sram;

    const double tcam_usage    = 100.0 - (stage.available_tcam * 100.0) / properties.tcam_per_stage;
    const bits_t tcam_consumed = properties.tcam_per_stage - stage.available_tcam;

    const double map_ram_usage    = 100.0 - (stage.available_map_ram * 100.0) / properties.map_ram_per_stage;
    const bits_t map_ram_consumed = properties.map_ram_per_stage - stage.available_map_ram;

    const double xbar_usage    = 100.0 - (stage.available_exact_match_xbar * 100.0) / properties.exact_match_xbar_per_stage;
    const bits_t xbar_consumed = properties.exact_match_xbar_per_stage - stage.available_exact_match_xbar;

    ss << "-------------------------------------\n";
    ss << "Stage " << stage.stage_id;
    ss << "\n";

    ss << "SRAM: ";
    ss << int2hr(sram_consumed / 8);
    ss << "/";
    ss << int2hr(properties.sram_per_stage / 8);
    ss << " B ";
    ss << "(" << sram_usage << "%)";
    ss << "\n";

    ss << "TCAM: ";
    ss << int2hr(tcam_consumed / 8);
    ss << "/";
    ss << int2hr(properties.tcam_per_stage / 8);
    ss << " B ";
    ss << "(" << tcam_usage << "%)";
    ss << "\n";

    ss << "MapRAM: ";
    ss << int2hr(map_ram_consumed / 8);
    ss << "/";
    ss << int2hr(properties.map_ram_per_stage / 8);
    ss << " B ";
    ss << "(" << map_ram_usage << "%)";
    ss << "\n";

    ss << "Exact Match Crossbar: ";
    ss << int2hr(xbar_consumed / 8);
    ss << "/";
    ss << int2hr(properties.exact_match_xbar_per_stage / 8);
    ss << " B ";
    ss << "(" << xbar_usage << "%)";
    ss << "\n";

    ss << "Objs: [";
    bool first = true;
    for (DS_ID ds_id : stage.data_structures) {
      if (!first) {
        ss << ",";
      }
      ss << ds_id;
      first = false;
    }
    ss << "]";
    ss << "\n";

    ss << "-------------------------------------\n";
  }

  ss << "==========================================================\n";

  std::cerr << ss.str();
}

int Pipeline::get_placed_stage(DS_ID ds_id) const {
  auto it = std::find_if(resources.stages.begin(), resources.stages.end(),
                         [ds_id](const Stage &stage) { return stage.data_structures.find(ds_id) != stage.data_structures.end(); });

  if (it != resources.stages.end()) {
    return it->stage_id;
  }

  return -1;
}

bool Pipeline::already_placed(DS_ID ds_id) const { return get_placed_stage(ds_id) >= 0; }

bool Pipeline::already_requested(DS_ID ds_id) const {
  return std::any_of(placement_requests.begin(), placement_requests.end(), [ds_id](const PlacementRequest &request) { return request.ds == ds_id; });
}

bool Pipeline::detect_changes_to_already_placed_data_structure(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  const std::vector<std::unordered_set<const DS *>> internal_primitive = ds->get_internal_primitive();

  if (!already_requested(ds->id)) {
    return false;
  }

  bool change = false;
  for (const std::unordered_set<const DS *> &independent_data_structures : internal_primitive) {
    for (const DS *independent_ds : independent_data_structures) {
      if (!already_placed(independent_ds->id)) {
        change = true;
        break;
      }
    }
  }

  return change;
}

int Pipeline::get_soonest_stage_satisfying_all_dependencies(const std::unordered_set<DS_ID> &deps) const {
  if (deps.empty()) {
    return 0;
  }

  std::unordered_set<DS_ID> cummulative_ds;
  int soonest_stage_id = -1;

  for (const Stage &stage : resources.stages) {
    cummulative_ds.insert(stage.data_structures.begin(), stage.data_structures.end());

    bool all_dependencies_are_satisfied =
        std::all_of(deps.begin(), deps.end(), [&cummulative_ds](DS_ID dep) { return cummulative_ds.find(dep) != cummulative_ds.end(); });

    if (all_dependencies_are_satisfied) {
      soonest_stage_id = stage.stage_id + 1;
      break;
    }
  }

  if (soonest_stage_id >= static_cast<int>(resources.stages.size())) {
    soonest_stage_id = -1;
  }

  return soonest_stage_id;
}

void Pipeline::place(const DS *ds, const std::unordered_set<DS_ID> &deps) {
  const bool duplicated_request = already_requested(ds->id);
  if (duplicated_request && !detect_changes_to_already_placed_data_structure(ds, deps)) {
    return;
  }

  const PlacementResult result = find_placements(ds, deps);
  if (result.status != PlacementStatus::Success) {
    debug();
    panic("Cannot place data structure %s: %s", ds->id.c_str(), placement_status_to_string(result.status).c_str());
  }

  assert_or_panic(result.resources.has_value(), "Placement result should have resources on success");
  resources = *result.resources;

  if (!duplicated_request) {
    placement_requests.push_back({ds->id, deps});
  }
}

PlacementStatus Pipeline::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  if (ds->primitive) {
    if (deps.find(ds->id) != deps.end()) {
      return PlacementStatus::SelfDependence;
    }
  }

  if (already_requested(ds->id) && !detect_changes_to_already_placed_data_structure(ds, deps)) {
    return PlacementStatus::Success;
  }

  return find_placements(ds, deps).status;
}

PlacementResult Pipeline::find_placements(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  PlacementResult result;

  result = SimplePlacer::find_placements(*this, ds, deps);
  if (result.status == PlacementStatus::Success) {
    return result;
  }

  // debug();
  // std::cerr << "Simple placer failed to place data structure: " << ds->id << "\n";
  // std::cerr << "Deps: [";
  // for (const DS_ID &dep : deps) {
  //   std::cerr << dep << ", ";
  // }
  // std::cerr << "]\n";
  // std::cerr << "Reason: " << placement_status_to_string(result.status) << "\n";

  return SolverPlacer::find_placements(*this, ds, deps);
}

} // namespace Tofino
} // namespace LibSynapse