#include "simple_placer.h"
#include "tna.h"

#include "../../../system.h"

namespace synapse {
namespace tofino {
namespace {
int get_soonest_available_stage(const std::vector<Stage> &stages, const std::unordered_set<DS_ID> &deps) {
  const Stage *soonest_stage = nullptr;

  for (auto it = stages.rbegin(); it != stages.rend(); it++) {
    const Stage *stage = &(*it);

    bool can_place = true;
    for (DS_ID dependency : deps) {
      if (stage->tables.find(dependency) != stage->tables.end()) {
        can_place = false;
        break;
      }
    }

    if (can_place) {
      soonest_stage = stage;
    }
  }

  if (soonest_stage) {
    return soonest_stage->stage_id;
  }

  return -1;
}

std::vector<Stage> create_stages(const TNAProperties *properties) {
  std::vector<Stage> stages;

  for (int stage_id = 0; stage_id < properties->stages; stage_id++) {
    Stage s = {
        .stage_id = stage_id,
        .available_sram = properties->sram_per_stage,
        .available_tcam = properties->tcam_per_stage,
        .available_map_ram = properties->map_ram_per_stage,
        .available_exact_match_xbar = properties->exact_match_xbar_per_stage,
        .available_logical_ids = properties->max_logical_sram_and_tcam_tables_per_stage,
        .tables = {},
    };

    stages.push_back(s);
  }

  return stages;
}
} // namespace

std::ostream &operator<<(std::ostream &os, const PlacementStatus &status) {
  switch (status) {
  case PlacementStatus::SUCCESS:
    os << "SUCCESS";
    break;
  case PlacementStatus::TOO_LARGE:
    os << "TOO_LARGE";
    break;
  case PlacementStatus::TOO_MANY_KEYS:
    os << "TOO_MANY_KEYS";
    break;
  case PlacementStatus::XBAR_CONSUME_EXCEEDS_LIMIT:
    os << "XBAR_CONSUME_EXCEEDS_LIMIT";
    break;
  case PlacementStatus::NO_AVAILABLE_STAGE:
    os << "NO_AVAILABLE_STAGE";
    break;
  case PlacementStatus::INCONSISTENT_PLACEMENT:
    os << "INCONSISTENT_PLACEMENT";
    break;
  case PlacementStatus::SELF_DEPENDENCE:
    os << "SELF_DEPENDENCE";
    break;
  case PlacementStatus::UNKNOWN:
    os << "UNKNOWN";
    break;
  }
  return os;
}

SimplePlacer::SimplePlacer(const TNAProperties *_properties) : properties(_properties), stages(create_stages(_properties)) {}

SimplePlacer::SimplePlacer(const SimplePlacer &other) : properties(other.properties), stages(other.stages) {
  for (const PlacementRequest &req : other.placement_requests) {
    placement_requests.push_back({req.ds->clone(), req.deps});
  }
}

SimplePlacer::~SimplePlacer() {
  for (const PlacementRequest &req : placement_requests) {
    delete req.ds;
  }
}

struct SimplePlacer::placement_t {
  int stage_id;
  bits_t sram;
  bits_t map_ram;
  bits_t xbar;
  int logical_ids;
  DS_ID obj;
};

void SimplePlacer::concretize_placement(Stage &stage, const SimplePlacer::placement_t &placement) {
  assert(stage.stage_id == placement.stage_id && "Invalid stage ID");
  assert(stage.available_sram >= placement.sram && "Not enough SRAM");
  assert(stage.available_map_ram >= placement.map_ram && "Not enough MAP RAM");
  assert(stage.available_exact_match_xbar >= placement.xbar && "Not enough XBAR");
  assert(stage.available_logical_ids >= placement.logical_ids && "Not enough logical IDs");

  stage.available_sram -= placement.sram;
  stage.available_map_ram -= placement.map_ram;
  stage.available_exact_match_xbar -= placement.xbar;
  stage.available_logical_ids -= placement.logical_ids;
  stage.tables.insert(placement.obj);
}

bool SimplePlacer::is_placed(DS_ID ds_id) const {
  for (const Stage &stage : stages) {
    if (stage.tables.find(ds_id) != stage.tables.end()) {
      return true;
    }
  }

  return false;
}

PlacementStatus SimplePlacer::is_consistent(DS_ID ds_id, const std::unordered_set<DS_ID> &deps) const {
  int soonest_stage_id = get_soonest_available_stage(stages, deps);

  int total_stages = stages.size();
  for (int stage_id = soonest_stage_id; stage_id < total_stages; stage_id++) {
    const Stage *stage = &stages[stage_id];

    if (stage->tables.find(ds_id) != stage->tables.end()) {
      return PlacementStatus::SUCCESS;
    }
  }

  return PlacementStatus::INCONSISTENT_PLACEMENT;
}

bool SimplePlacer::is_self_dependent(DS_ID ds_id, const std::unordered_set<DS_ID> &deps) const { return deps.find(ds_id) != deps.end(); }

PlacementStatus SimplePlacer::find_placements(const DS *ds, const std::unordered_set<DS_ID> &deps,
                                              std::vector<placement_t> &placements) const {
  PlacementStatus status;
  switch (ds->type) {
  case DSType::TABLE:
    status = find_placements_table(dynamic_cast<const Table *>(ds), deps, placements);
    break;
  case DSType::REGISTER:
    status = find_placements_reg(dynamic_cast<const Register *>(ds), deps, placements);
    break;
  case DSType::METER:
    status = find_placements_meter(dynamic_cast<const Meter *>(ds), deps, placements);
    break;
  case DSType::HASH:
    status = find_placements_hash(dynamic_cast<const Hash *>(ds), deps, placements);
    break;
  default:
    panic("Unsupported DS type");
  }

  return status;
}

PlacementStatus SimplePlacer::find_placements_table(const Table *table, const std::unordered_set<DS_ID> &deps,
                                                    std::vector<placement_t> &placements) const {
  assert(!is_placed(table->id) && "Table already placed");

  if (static_cast<int>(table->keys.size()) > properties->max_exact_match_keys) {
    return PlacementStatus::TOO_MANY_KEYS;
  }

  if (table->get_match_xbar_consume() > properties->exact_match_xbar_per_stage) {
    return PlacementStatus::XBAR_CONSUME_EXCEEDS_LIMIT;
  }

  int soonest_stage_id = get_soonest_available_stage(stages, deps);
  assert(soonest_stage_id < static_cast<int>(stages.size()) && "No available stage");

  if (soonest_stage_id < 0) {
    return PlacementStatus::NO_AVAILABLE_STAGE;
  }

  bits_t requested_sram = align_to_byte(table->get_consumed_sram());
  bits_t requested_xbar = align_to_byte(table->get_match_xbar_consume());

  int total_stages = stages.size();
  for (int stage_id = soonest_stage_id; stage_id < total_stages; stage_id++) {
    const Stage &stage = stages[stage_id];

    if (stage.available_sram == 0) {
      continue;
    }

    if (requested_xbar > stage.available_exact_match_xbar) {
      continue;
    }

    if (stage.available_logical_ids == 0) {
      continue;
    }

    // This is not actually how it happens, but this is a VERY simple placer.
    bits_t amount_placed = std::min(requested_sram, stage.available_sram);

    placement_t placement = {
        .stage_id = stage_id,
        .sram = amount_placed,
        .map_ram = 0,
        .xbar = requested_xbar,
        .logical_ids = 1,
        .obj = table->id,
    };

    requested_sram -= amount_placed;
    placements.push_back(placement);

    if (requested_sram == 0) {
      break;
    }
  }

  if (requested_sram > 0) {
    return PlacementStatus::TOO_LARGE;
  }

  return PlacementStatus::SUCCESS;
}

PlacementStatus SimplePlacer::find_placements_reg(const Register *reg, const std::unordered_set<DS_ID> &deps,
                                                  std::vector<placement_t> &placements) const {
  assert(!is_placed(reg->id) && "Register already placed");

  int soonest_stage_id = get_soonest_available_stage(stages, deps);
  assert(soonest_stage_id < static_cast<int>(stages.size()) && "No available stage");

  if (soonest_stage_id < 0) {
    return PlacementStatus::NO_AVAILABLE_STAGE;
  }

  bits_t requested_sram = align_to_byte(reg->get_consumed_sram());
  bits_t requested_map_ram = align_to_byte(requested_sram);
  bits_t requested_xbar = align_to_byte(reg->index_size);
  int requested_logical_ids = reg->get_num_logical_ids();

  int total_stages = stages.size();
  for (int stage_id = soonest_stage_id; stage_id < total_stages; stage_id++) {
    const Stage &stage = stages[stage_id];

    // Note that for tables we can span them across multiple stages, but we
    // can't do that with registers.
    if (requested_sram > stage.available_sram) {
      continue;
    }

    if (requested_map_ram > stage.available_map_ram) {
      continue;
    }

    if (requested_xbar > stage.available_exact_match_xbar) {
      continue;
    }

    if (stage.available_logical_ids < requested_logical_ids) {
      continue;
    }

    placement_t placement = {
        .stage_id = stage_id,
        .sram = requested_sram,
        .map_ram = requested_map_ram,
        .xbar = requested_xbar,
        .logical_ids = requested_logical_ids,
        .obj = reg->id,
    };

    requested_sram = 0;
    placements.push_back(placement);

    break;
  }

  if (requested_sram > 0) {
    return PlacementStatus::TOO_LARGE;
  }

  return PlacementStatus::SUCCESS;
}

PlacementStatus SimplePlacer::find_placements_meter(const Meter *meter, const std::unordered_set<DS_ID> &deps,
                                                    std::vector<placement_t> &placements) const {
  assert(!is_placed(meter->id) && "Meter already placed");

  if (static_cast<int>(meter->keys.size()) > properties->max_exact_match_keys) {
    return PlacementStatus::TOO_MANY_KEYS;
  }

  if (meter->get_match_xbar_consume() > properties->exact_match_xbar_per_stage) {
    return PlacementStatus::XBAR_CONSUME_EXCEEDS_LIMIT;
  }

  int soonest_stage_id = get_soonest_available_stage(stages, deps);
  assert(soonest_stage_id < static_cast<int>(stages.size()) && "No available stage");

  if (soonest_stage_id < 0) {
    return PlacementStatus::NO_AVAILABLE_STAGE;
  }

  bits_t requested_sram = align_to_byte(meter->get_consumed_sram());
  bits_t requested_xbar = align_to_byte(meter->get_match_xbar_consume());

  int total_stages = stages.size();
  for (int stage_id = soonest_stage_id; stage_id < total_stages; stage_id++) {
    const Stage &stage = stages[stage_id];

    if (stage.available_sram == 0) {
      continue;
    }

    if (requested_xbar > stage.available_exact_match_xbar) {
      continue;
    }

    if (stage.available_logical_ids == 0) {
      continue;
    }

    // This is not actually how it happens, but this is a VERY simple placer.
    bits_t amount_placed = std::min(requested_sram, stage.available_sram);

    placement_t placement = {
        .stage_id = stage_id,
        .sram = amount_placed,
        .map_ram = 0,
        .xbar = requested_xbar,
        .logical_ids = 1,
        .obj = meter->id,
    };

    requested_sram -= amount_placed;
    placements.push_back(placement);

    if (requested_sram == 0) {
      break;
    }
  }

  if (requested_sram > 0) {
    return PlacementStatus::TOO_LARGE;
  }

  return PlacementStatus::SUCCESS;
}

PlacementStatus SimplePlacer::find_placements_hash(const Hash *hash, const std::unordered_set<DS_ID> &deps,
                                                   std::vector<placement_t> &placements) const {
  assert(!is_placed(hash->id) && "Hash already placed");

  if (static_cast<int>(hash->keys.size()) > properties->max_exact_match_keys) {
    return PlacementStatus::TOO_MANY_KEYS;
  }

  if (hash->get_match_xbar_consume() > properties->exact_match_xbar_per_stage) {
    return PlacementStatus::XBAR_CONSUME_EXCEEDS_LIMIT;
  }

  int soonest_stage_id = get_soonest_available_stage(stages, deps);
  assert(soonest_stage_id < static_cast<int>(stages.size()) && "No available stage");

  if (soonest_stage_id < 0) {
    return PlacementStatus::NO_AVAILABLE_STAGE;
  }

  bits_t requested_xbar = align_to_byte(hash->get_match_xbar_consume());

  int total_stages = stages.size();
  for (int stage_id = soonest_stage_id; stage_id < total_stages; stage_id++) {
    const Stage &stage = stages[stage_id];

    if (requested_xbar > stage.available_exact_match_xbar) {
      continue;
    }

    if (stage.available_logical_ids == 0) {
      continue;
    }

    placement_t placement = {
        .stage_id = stage_id,
        .sram = 0,
        .map_ram = 0,
        .xbar = requested_xbar,
        .logical_ids = 1,
        .obj = hash->id,
    };

    placements.push_back(placement);
    break;
  }

  return PlacementStatus::SUCCESS;
}

void SimplePlacer::place(const DS *ds, const std::unordered_set<DS_ID> &deps) {
  if (ds->primitive) {
    place_primitive_ds(ds, deps);
  } else {
    bool already_placed = false;
    for (const PlacementRequest &req : placement_requests) {
      if (req.ds->id == ds->id) {
        already_placed = true;
        break;
      }
    }

    bool change = false;
    if (already_placed) {
      for (const std::unordered_set<const DS *> &indep_ds : ds->get_internal_primitive()) {
        for (const DS *ds : indep_ds) {
          if (!is_placed(ds->id)) {
            change = true;
            break;
          }
        }
      }
    }

    if (change) {
      SimplePlacer clean_snapshot(properties);

      for (PlacementRequest req : placement_requests) {
        if (req.ds->id == ds->id) {
          req.ds = ds;
          req.deps.insert(deps.begin(), deps.end());
        }

        clean_snapshot.place(req.ds, req.deps);
      }

      replace_placement_request(ds, deps, clean_snapshot.stages);
      return;
    } else {
      std::unordered_set<DS_ID> current_deps = deps;
      for (const std::unordered_set<const DS *> &indep_ds : ds->get_internal_primitive()) {
        std::unordered_set<DS_ID> new_deps;

        for (const DS *ds : indep_ds) {
          if (is_placed(ds->id)) {
            continue;
          }

          place_primitive_ds(ds, current_deps);
          new_deps.insert(ds->id);
        }

        current_deps.insert(new_deps.begin(), new_deps.end());
      }
    }
  }

  save_placement_request(ds, deps);
}

void SimplePlacer::place_primitive_ds(const DS *ds, const std::unordered_set<DS_ID> &deps) {
  assert(ds->primitive && "DS is not primitive");

  if (is_placed(ds->id)) {
    return;
  }

  std::vector<placement_t> placements;
  PlacementStatus status = find_placements(ds, deps, placements);
  assert(status == PlacementStatus::SUCCESS && "Cannot place ds");

  for (const placement_t &placement : placements) {
    assert(placement.stage_id < static_cast<int>(stages.size()) && "Invalid stage");
    concretize_placement(stages[placement.stage_id], placement);
  }
}

PlacementStatus SimplePlacer::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  PlacementStatus status = PlacementStatus::UNKNOWN;

  if (ds->primitive) {
    if (is_self_dependent(ds->id, deps)) {
      return PlacementStatus::SELF_DEPENDENCE;
    }

    if (is_placed(ds->id)) {
      return is_consistent(ds->id, deps);
    }

    std::vector<placement_t> placements;
    return find_placements(ds, deps, placements);
  }

  std::vector<std::unordered_set<const DS *>> candidates = ds->get_internal_primitive();

  bool already_placed = false;
  for (const PlacementRequest &req : placement_requests) {
    if (req.ds->id == ds->id) {
      already_placed = true;
      break;
    }
  }

  bool change = false;
  if (already_placed) {
    for (const std::unordered_set<const DS *> &indep_ds : candidates) {
      for (const DS *ds : indep_ds) {
        if (!is_placed(ds->id)) {
          change = true;
          break;
        }
      }
    }
  }

  if (change) {
    SimplePlacer clean_snapshot(properties);

    for (PlacementRequest req : placement_requests) {
      if (req.ds->id == ds->id) {
        req.ds = ds;
        req.deps.insert(deps.begin(), deps.end());
      }

      status = clean_snapshot.can_place(req.ds, req.deps);

      if (status != PlacementStatus::SUCCESS) {
        return status;
      }

      clean_snapshot.place(req.ds, req.deps);
    }
  } else {
    SimplePlacer snapshot = *this;

    std::unordered_set<DS_ID> current_deps = deps;
    for (const std::unordered_set<const DS *> &indep_ds : candidates) {
      std::unordered_set<DS_ID> new_deps;

      for (const DS *ds : indep_ds) {
        PlacementStatus status = snapshot.can_place(ds, current_deps);

        if (status != PlacementStatus::SUCCESS) {
          return status;
        }

        snapshot.place(ds, current_deps);
        new_deps.insert(ds->id);
      }

      current_deps.insert(new_deps.begin(), new_deps.end());
    }
  }

  return PlacementStatus::SUCCESS;
}

void SimplePlacer::save_placement_request(const DS *ds, const std::unordered_set<DS_ID> &deps) {
  placement_requests.push_back({ds->clone(), deps});
}

void SimplePlacer::replace_placement_request(const DS *ds, const std::unordered_set<DS_ID> &deps, const std::vector<Stage> &new_stages) {
  stages = new_stages;

  for (PlacementRequest &req : placement_requests) {
    if (req.ds->id == ds->id) {
      delete req.ds;
      req.ds = ds->clone();
      req.deps.insert(deps.begin(), deps.end());
      return;
    }
  }

  panic("Cannot replace placement request (not found)");
}

void SimplePlacer::debug() const {
  std::cerr << "\n";
  std::cerr << "====================== SimplePlacer ======================\n";

  for (const Stage &stage : stages) {
    if (stage.tables.empty()) {
      continue;
    }

    std::stringstream ss;

    double sram_usage = 100.0 - (stage.available_sram * 100.0) / properties->sram_per_stage;
    bits_t sram_consumed = properties->sram_per_stage - stage.available_sram;

    double tcam_usage = 100.0 - (stage.available_tcam * 100.0) / properties->tcam_per_stage;
    bits_t tcam_consumed = properties->tcam_per_stage - stage.available_tcam;

    double map_ram_usage = 100.0 - (stage.available_map_ram * 100.0) / properties->map_ram_per_stage;
    bits_t map_ram_consumed = properties->map_ram_per_stage - stage.available_map_ram;

    double xbar_usage = 100.0 - (stage.available_exact_match_xbar * 100.0) / properties->exact_match_xbar_per_stage;
    bits_t xbar_consumed = properties->exact_match_xbar_per_stage - stage.available_exact_match_xbar;

    ss << "-------------------------------------\n";
    ss << "Stage " << stage.stage_id;
    ss << "\n";

    ss << "SRAM: ";
    ss << int2hr(sram_consumed / 8);
    ss << "/";
    ss << int2hr(properties->sram_per_stage / 8);
    ss << " B ";
    ss << "(" << sram_usage << "%)";
    ss << "\n";

    ss << "TCAM: ";
    ss << int2hr(tcam_consumed / 8);
    ss << "/";
    ss << int2hr(properties->tcam_per_stage / 8);
    ss << " B ";
    ss << "(" << tcam_usage << "%)";
    ss << "\n";

    ss << "MapRAM: ";
    ss << int2hr(map_ram_consumed / 8);
    ss << "/";
    ss << int2hr(properties->map_ram_per_stage / 8);
    ss << " B ";
    ss << "(" << map_ram_usage << "%)";
    ss << "\n";

    ss << "Exact Match Crossbar: ";
    ss << int2hr(xbar_consumed / 8);
    ss << "/";
    ss << int2hr(properties->exact_match_xbar_per_stage / 8);
    ss << " B ";
    ss << "(" << xbar_usage << "%)";
    ss << "\n";

    ss << "Objs: [";
    bool first = true;
    for (DS_ID table_id : stage.tables) {
      if (!first) {
        ss << ",";
      }
      ss << table_id;
      first = false;
    }
    ss << "]";
    ss << "\n";

    ss << "-------------------------------------\n";

    std::cerr << ss.str();
  }

  std::cerr << "==========================================================\n";
}

} // namespace tofino
} // namespace synapse