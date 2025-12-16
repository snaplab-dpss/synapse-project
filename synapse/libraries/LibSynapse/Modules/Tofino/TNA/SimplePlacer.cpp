#include <LibSynapse/Modules/Tofino/TNA/SimplePlacer.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

namespace LibSynapse {
namespace Tofino {
namespace SimplePlacer {

struct Placement {
  int stage_id;
  bits_t sram;
  bits_t tcam;
  bits_t map_ram;
  bits_t xbar;
  int logical_ids;
  DS_ID obj;
  DSType type;
};

bool detect_changes_to_already_placed_data_structure(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps) {
  const std::vector<std::unordered_set<const DS *>> internal_primitive = ds->get_internal_primitive();

  if (!pipeline.already_requested(ds->id)) {
    return false;
  }

  bool change = false;
  for (const std::unordered_set<const DS *> &independent_data_structures : internal_primitive) {
    for (const DS *independent_ds : independent_data_structures) {
      if (!pipeline.already_placed(independent_ds->id)) {
        change = true;
        break;
      }
    }
  }

  return change;
}

PlacementResult concretize_placements(const Pipeline &pipeline, const PlacementRequest &request, const std::vector<Placement> &placements) {
  PipelineResources resources = pipeline.resources;

  for (const Placement &placement : placements) {
    assert(placement.stage_id >= 0 && placement.stage_id < static_cast<int>(resources.stages.size()) && "Invalid stage ID in placement");
    Stage &stage = resources.stages[placement.stage_id];

    assert(stage.stage_id == placement.stage_id && "Invalid stage ID");
    assert(stage.available_sram >= placement.sram && "Not enough SRAM");
    assert(stage.available_map_ram >= placement.map_ram && "Not enough MAP RAM");
    assert(stage.available_exact_match_xbar >= placement.xbar && "Not enough XBAR");
    assert(stage.available_logical_ids >= placement.logical_ids && "Not enough logical IDs");

    stage.available_sram -= placement.sram;
    stage.available_map_ram -= placement.map_ram;
    stage.available_exact_match_xbar -= placement.xbar;
    stage.available_logical_ids -= placement.logical_ids;
    stage.data_structures.insert(placement.obj);

    if (placement.type == DSType::Digest) {
      resources.used_digests++;
    }
  }

  return resources;
}

PlacementResult clean_slate_placement(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps) {
  Pipeline clean_slate_pipeline(pipeline.properties, pipeline.data_structures);

  for (PlacementRequest req : pipeline.placement_requests) {
    const DS *requested_ds = clean_slate_pipeline.data_structures.get_ds_from_id(req.ds);

    if (requested_ds->id == ds->id) {
      req.deps.insert(deps.begin(), deps.end());
    }

    const PlacementResult result = find_placements(clean_slate_pipeline, requested_ds, req.deps);
    if (result.status != PlacementStatus::Success) {
      return result;
    }

    clean_slate_pipeline.resources = *result.resources;
  }

  return clean_slate_pipeline.resources;
}

PlacementResult find_placements(const Pipeline &pipeline, const DS *ds, const std::unordered_set<DS_ID> &deps) {
  if (ds->primitive) {
    PlacementResult result;
    switch (ds->type) {
    case DSType::Table:
      result = find_placements_table(pipeline, dynamic_cast<const Table *>(ds), deps);
      break;
    case DSType::Register:
      result = find_placements_reg(pipeline, dynamic_cast<const Register *>(ds), deps);
      break;
    case DSType::Meter:
      result = find_placements_meter(pipeline, dynamic_cast<const Meter *>(ds), deps);
      break;
    case DSType::Hash:
      result = find_placements_hash(pipeline, dynamic_cast<const Hash *>(ds), deps);
      break;
    case DSType::Digest:
      result = find_placements_digest(pipeline, dynamic_cast<const Digest *>(ds), deps);
      break;
    case DSType::LPM:
      result = find_placements_lpm(pipeline, dynamic_cast<const LPM *>(ds), deps);
      break;
    default:
      panic("Unhandled DS type: %s", ds->id.c_str());
    }
    return result;
  }

  if (detect_changes_to_already_placed_data_structure(pipeline, ds, deps)) {
    return clean_slate_placement(pipeline, ds, deps);
  }

  Pipeline snapshot = pipeline;

  std::unordered_set<DS_ID> current_deps = deps;
  for (const std::unordered_set<const DS *> &independent_data_structures : ds->get_internal_primitive()) {
    std::unordered_set<DS_ID> new_deps;

    for (const DS *independent_ds : independent_data_structures) {
      const PlacementResult result = find_placements(snapshot, independent_ds, current_deps);
      if (result.status != PlacementStatus::Success) {
        return result;
      }

      snapshot.resources = *result.resources;
      new_deps.insert(independent_ds->id);
    }

    current_deps.insert(new_deps.begin(), new_deps.end());
  }

  return snapshot.resources;
}

PlacementResult find_placements_table(const Pipeline &pipeline, const Table *table, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(table->id) && "Table already placed");

  if (static_cast<int>(table->keys.size()) > pipeline.properties.max_exact_match_keys) {
    return PlacementStatus::TooManyKeys;
  }

  if (table->get_match_xbar_consume() > pipeline.properties.exact_match_xbar_per_stage) {
    return PlacementStatus::XBarConsumptionExceedsLimit;
  }

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);

  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  bits_t requested_sram = align_to_byte(table->get_consumed_sram());
  bits_t requested_xbar = align_to_byte(table->get_match_xbar_consume());

  std::vector<Placement> placements;

  for (const Stage &stage : pipeline.resources.stages) {
    if (stage.stage_id < soonest_stage_id) {
      continue;
    }

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

    const Placement placement = {
        .stage_id    = stage.stage_id,
        .sram        = amount_placed,
        .tcam        = 0,
        .map_ram     = 0,
        .xbar        = requested_xbar,
        .logical_ids = 1,
        .obj         = table->id,
        .type        = table->type,
    };

    requested_sram -= amount_placed;
    placements.push_back(placement);

    if (requested_sram == 0) {
      break;
    }
  }

  if (requested_sram > 0) {
    return PlacementStatus::TooLarge;
  }

  return concretize_placements(pipeline, {table->id, deps}, placements);
}

PlacementResult find_placements_reg(const Pipeline &pipeline, const Register *reg, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(reg->id) && "Register already placed");

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  bits_t requested_sram     = align_to_byte(reg->get_consumed_sram());
  bits_t requested_map_ram  = align_to_byte(requested_sram);
  bits_t requested_xbar     = align_to_byte(reg->index_size);
  int requested_logical_ids = reg->get_num_logical_ids();

  std::vector<Placement> placements;

  for (const Stage &stage : pipeline.resources.stages) {
    if (stage.stage_id < soonest_stage_id) {
      continue;
    }

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

    if (requested_logical_ids > stage.available_logical_ids) {
      continue;
    }

    const Placement placement = {
        .stage_id    = stage.stage_id,
        .sram        = requested_sram,
        .tcam        = 0,
        .map_ram     = requested_map_ram,
        .xbar        = requested_xbar,
        .logical_ids = requested_logical_ids,
        .obj         = reg->id,
        .type        = reg->type,
    };

    requested_sram = 0;
    placements.push_back(placement);

    break;
  }

  if (requested_sram > 0) {
    return PlacementStatus::TooLarge;
  }

  return concretize_placements(pipeline, {reg->id, deps}, placements);
}

PlacementResult find_placements_meter(const Pipeline &pipeline, const Meter *meter, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(meter->id) && "Meter already placed");

  if (static_cast<int>(meter->keys.size()) > pipeline.properties.max_exact_match_keys) {
    return PlacementStatus::TooManyKeys;
  }

  if (meter->get_match_xbar_consume() > pipeline.properties.exact_match_xbar_per_stage) {
    return PlacementStatus::XBarConsumptionExceedsLimit;
  }

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  bits_t requested_sram = align_to_byte(meter->get_consumed_sram());
  bits_t requested_xbar = align_to_byte(meter->get_match_xbar_consume());

  std::vector<Placement> placements;

  for (const Stage &stage : pipeline.resources.stages) {
    if (stage.stage_id < soonest_stage_id) {
      continue;
    }

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
    const bits_t amount_placed = std::min(requested_sram, stage.available_sram);

    const Placement placement = {
        .stage_id    = stage.stage_id,
        .sram        = amount_placed,
        .tcam        = 0,
        .map_ram     = 0,
        .xbar        = requested_xbar,
        .logical_ids = 1,
        .obj         = meter->id,
        .type        = meter->type,
    };

    requested_sram -= amount_placed;
    placements.push_back(placement);

    if (requested_sram == 0) {
      break;
    }
  }

  if (requested_sram > 0) {
    return PlacementStatus::TooLarge;
  }

  return concretize_placements(pipeline, {meter->id, deps}, placements);
}

PlacementResult find_placements_hash(const Pipeline &pipeline, const Hash *hash, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(hash->id) && "Hash already placed");

  if (static_cast<int>(hash->keys.size()) > pipeline.properties.max_exact_match_keys) {
    return PlacementStatus::TooManyKeys;
  }

  if (hash->get_match_xbar_consume() > pipeline.properties.exact_match_xbar_per_stage) {
    return PlacementStatus::XBarConsumptionExceedsLimit;
  }

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  const bits_t requested_xbar = align_to_byte(hash->get_match_xbar_consume());

  std::vector<Placement> placements;

  for (const Stage &stage : pipeline.resources.stages) {
    if (stage.stage_id < soonest_stage_id) {
      continue;
    }

    if (requested_xbar > stage.available_exact_match_xbar) {
      continue;
    }

    if (stage.available_logical_ids == 0) {
      continue;
    }

    const Placement placement = {
        .stage_id    = stage.stage_id,
        .sram        = 0,
        .tcam        = 0,
        .map_ram     = 0,
        .xbar        = requested_xbar,
        .logical_ids = 1,
        .obj         = hash->id,
        .type        = hash->type,
    };

    placements.push_back(placement);
    break;
  }

  if (placements.empty()) {
    return PlacementStatus::MultipleReasons;
  }

  return concretize_placements(pipeline, {hash->id, deps}, placements);
}

PlacementResult find_placements_digest(const Pipeline &pipeline, const Digest *digest, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(digest->id) && "Digest already placed");

  if (pipeline.resources.used_digests >= pipeline.properties.max_digests) {
    return PlacementStatus::NotEnoughDigests;
  }

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  const Placement placement = {
      .stage_id    = soonest_stage_id,
      .sram        = 0,
      .tcam        = 0,
      .map_ram     = 0,
      .xbar        = 0,
      .logical_ids = 0,
      .obj         = digest->id,
      .type        = digest->type,
  };

  return concretize_placements(pipeline, {digest->id, deps}, {placement});
}

PlacementResult find_placements_lpm(const Pipeline &pipeline, const LPM *lpm, const std::unordered_set<DS_ID> &deps) {
  assert(!pipeline.already_placed(lpm->id) && "LPM already placed");

  if (lpm->get_match_xbar_consume() > pipeline.properties.exact_match_xbar_per_stage) {
    return PlacementStatus::XBarConsumptionExceedsLimit;
  }

  const int soonest_stage_id = pipeline.get_soonest_stage_satisfying_all_dependencies(deps);
  if (soonest_stage_id < 0) {
    return PlacementStatus::UnmetDependencies;
  }

  bits_t requested_tcam = align_to_byte(lpm->get_consumed_tcam());
  bits_t requested_xbar = align_to_byte(lpm->get_match_xbar_consume());

  std::vector<Placement> placements;

  for (const Stage &stage : pipeline.resources.stages) {
    if (stage.stage_id < soonest_stage_id) {
      continue;
    }

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
    const bits_t amount_placed = std::min(requested_tcam, stage.available_tcam);

    const Placement placement = {
        .stage_id    = stage.stage_id,
        .sram        = 0,
        .tcam        = amount_placed,
        .map_ram     = 0,
        .xbar        = requested_xbar,
        .logical_ids = 1,
        .obj         = lpm->id,
        .type        = lpm->type,
    };

    requested_tcam -= amount_placed;
    placements.push_back(placement);

    if (requested_tcam == 0) {
      break;
    }
  }

  if (requested_tcam > 0) {
    return PlacementStatus::TooLarge;
  }

  return concretize_placements(pipeline, {lpm->id, deps}, placements);
}

} // namespace SimplePlacer
} // namespace Tofino
} // namespace LibSynapse