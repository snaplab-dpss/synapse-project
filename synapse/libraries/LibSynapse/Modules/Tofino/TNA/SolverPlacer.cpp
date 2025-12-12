#include <LibSynapse/Modules/Tofino/TNA/SolverPlacer.h>

#include <z3++.h>

#ifdef USE_GUROBI_SOLVER
#include <gurobi_c++.h>
#endif

namespace LibSynapse {
namespace Tofino {
namespace SolverPlacer {

namespace {

struct ConcretizedPlacementRequest {
  const DS *ds;
  std::unordered_set<DS_ID> deps;
};

struct ds_relationships_t {
  std::unordered_set<const DS *> primitive_data_structures;
  std::vector<std::pair<DS_ID, DS_ID>> happens_before;
};

ds_relationships_t get_ds_relationships(const Pipeline &pipeline, const DS *target_ds, const std::unordered_set<DS_ID> &deps) {
  std::vector<ConcretizedPlacementRequest> placement_requests;

  for (const PlacementRequest &request : pipeline.placement_requests) {
    const DS *request_ds = pipeline.data_structures.get_ds_from_id(request.ds);
    placement_requests.push_back({request_ds, request.deps});
  }
  if (!pipeline.already_requested(target_ds->id)) {
    placement_requests.push_back({target_ds, deps});
  }

  ds_relationships_t ds_relationships;

  for (const ConcretizedPlacementRequest &request : placement_requests) {
    std::unordered_set<DS_ID> cummulative_deps;
    for (DS_ID dep_id : request.deps) {
      const DS *dep                         = pipeline.data_structures.get_ds_from_id(dep_id);
      const std::vector<DS_ID> ds_unwrapped = dep->unwrap();
      cummulative_deps.insert(ds_unwrapped.begin(), ds_unwrapped.end());
    }

    if (request.ds->primitive) {
      ds_relationships.primitive_data_structures.insert(request.ds);
      for (DS_ID dep : cummulative_deps) {
        ds_relationships.happens_before.emplace_back(dep, request.ds->id);
      }
      continue;
    }

    for (const std::unordered_set<const DS *> &data_structures : request.ds->get_internal_primitive()) {
      for (const DS *ds : data_structures) {
        for (DS_ID dep : cummulative_deps) {
          ds_relationships.happens_before.emplace_back(dep, ds->id);
        }
      }

      ds_relationships.primitive_data_structures.insert(data_structures.begin(), data_structures.end());

      for (const DS *ds : data_structures) {
        cummulative_deps.insert(ds->id);
      }
    }
  }

  return ds_relationships;
}

} // namespace

#ifdef USE_GUROBI_SOLVER
ActiveSolver get_active_solver() { return ActiveSolver::Gurobi; }

PlacementResult find_placements(const Pipeline &pipeline, const DS *target_ds, const std::unordered_set<DS_ID> &deps) {
  const ds_relationships_t ds_relationships = get_ds_relationships(pipeline, target_ds, deps);
  PipelineResources pipeline_resources(pipeline.properties);

  GRBEnv env = GRBEnv(true);
  env.set(GRB_IntParam_OutputFlag, 0);
  env.start();

  GRBModel model = GRBModel(env);

  model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

  const int TotalStages         = pipeline.properties.stages;
  const int MaxLogicalIDs       = pipeline.properties.max_logical_sram_and_tcam_tables_per_stage;
  const int TotalXbarPerStage   = pipeline.properties.exact_match_xbar_per_stage;
  const int TotalSRAMPerStage   = pipeline.properties.sram_per_stage;
  const int TotalTCAMPerStage   = pipeline.properties.tcam_per_stage;
  const int TotalMapRAMPerStage = pipeline.properties.map_ram_per_stage;
  const int MaxDigests          = pipeline.properties.max_digests;

  std::map<std::pair<int, DS_ID>, GRBVar> p_s_t;
  std::map<std::pair<int, DS_ID>, GRBVar> e_s_t;
  std::map<DS_ID, GRBVar> f_t;
  std::map<DS_ID, GRBVar> l_t;
  std::map<std::pair<int, DS_ID>, GRBVar> f_s_t_aux;
  std::map<std::pair<int, DS_ID>, GRBVar> l_s_t_aux;

  for (int s = 0; s < TotalStages; s++) {
    for (const DS *ds : ds_relationships.primitive_data_structures) {
      std::stringstream p_ss, e_ss;

      p_ss << "p_" << s << "_" << ds;
      e_ss << "e_" << s << "_" << ds;

      GRBVar p = model.addVar(0, 1, 0, GRB_BINARY, p_ss.str());
      GRBVar e = model.addVar(0, GRB_INFINITY, 0, GRB_INTEGER, e_ss.str());

      p_s_t.insert({{s, ds->id}, p});
      e_s_t.insert({{s, ds->id}, e});
    }
  }

  for (const DS *ds : ds_relationships.primitive_data_structures) {
    std::stringstream f_ss, l_ss;

    f_ss << "f_" << ds;
    l_ss << "l_" << ds;

    GRBVar l = model.addVar(0, TotalStages - 1, 1, GRB_INTEGER, l_ss.str());
    GRBVar f = model.addVar(0, TotalStages - 1, 0, GRB_INTEGER, f_ss.str());

    f_t.insert({ds->id, f});
    l_t.insert({ds->id, l});

    model.addConstr(f <= l, "f_le_l_" + ds->id);
  }

  for (const DS *ds : ds_relationships.primitive_data_structures) {
    for (int s = 0; s < TotalStages; s++) {
      std::stringstream f_s_t_aux_ss, l_s_t_aux_ss;

      f_s_t_aux_ss << "f_" << s << "_" << ds << "_aux";
      l_s_t_aux_ss << "l_" << s << "_" << ds << "_aux";

      GRBVar f_aux = model.addVar(0, TotalStages - 1, 0, GRB_INTEGER, f_s_t_aux_ss.str());
      GRBVar l_aux = model.addVar(0, TotalStages - 1, 0, GRB_INTEGER, l_s_t_aux_ss.str());

      f_s_t_aux.insert({{s, ds->id}, f_aux});
      l_s_t_aux.insert({{s, ds->id}, l_aux});

      model.addConstr(f_aux <= p_s_t.at({s, ds->id}), "f_aux_le_p_" + ds->id + "_s" + std::to_string(s));
      model.addConstr(l_aux <= p_s_t.at({s, ds->id}), "l_aux_le_p_" + ds->id + "_s" + std::to_string(s));
    }
  }

  // ***************************
  // Assignment constraints
  // ***************************

  // Allocating all entries for each data structure.
  // Also, allocating means placing.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    u32 capacity                  = 0;
    bool can_span_multiple_stages = false;

    switch (ds->type) {
    case DSType::Table: {
      const Table *table       = dynamic_cast<const Table *>(ds);
      capacity                 = table->capacity;
      can_span_multiple_stages = true;
    } break;
    case DSType::Register: {
      const Register *reg = dynamic_cast<const Register *>(ds);
      capacity            = reg->capacity;
    } break;
    case DSType::Meter: {
      const Meter *meter = dynamic_cast<const Meter *>(ds);
      capacity           = meter->capacity;
    } break;
    case DSType::Hash: {
      // Assume the hash takes 1 entry. This entry is meaningless.
      capacity = 1;
    } break;
    case DSType::Digest: {
      // Assume the digest takes 1 entry. This entry is meaningless.
      capacity = 1;
    } break;
    case DSType::LPM: {
      const LPM *lpm = dynamic_cast<const LPM *>(ds);
      capacity       = lpm->capacity;
    } break;
    default: {
      panic("Unexpected data structure type: %s", ds->id.c_str());
    }
    }

    GRBLinExpr e_t = 0;
    for (int s = 0; s < TotalStages; s++) {
      e_t = e_t + e_s_t.at({s, ds->id});
    }
    model.addConstr(e_t == capacity, "e_total_" + ds->id);

    for (int s = 0; s < TotalStages; s++) {
      const GRBVar &e = e_s_t.at({s, ds->id});
      const GRBVar &p = p_s_t.at({s, ds->id});
      model.addConstr(e <= capacity * p);
      model.addConstr(e >= p);
    }

    if (!can_span_multiple_stages) {
      GRBLinExpr p_sum = 0;
      for (int s = 0; s < TotalStages; s++) {
        p_sum = p_sum + p_s_t.at({s, ds->id});
      }
      model.addConstr(p_sum == 1, "p_sum_" + ds->id);
    }
  }

  // ***************************
  // Capacity constraints
  // ***************************

  std::vector<GRBLinExpr> sram_used_per_stage(TotalStages, 0);
  std::vector<GRBLinExpr> tcam_used_per_stage(TotalStages, 0);
  std::vector<GRBLinExpr> map_ram_used_per_stage(TotalStages, 0);
  std::vector<GRBLinExpr> xbar_used_per_stage(TotalStages, 0);
  std::vector<GRBLinExpr> logical_ids_used_per_stage(TotalStages, 0);

  for (int s = 0; s < TotalStages; s++) {
    for (const DS *ds : ds_relationships.primitive_data_structures) {
      u32 ds_sram_per_entry_used    = 0;
      u32 ds_tcam_per_entry_used    = 0;
      u32 ds_map_ram_per_entry_used = 0;
      u32 ds_xbar_used              = 0;
      u32 ds_logical_ids_used       = 0;

      switch (ds->type) {
      case DSType::Table: {
        const Table *table        = dynamic_cast<const Table *>(ds);
        ds_sram_per_entry_used    = align_to_byte(table->get_consumed_sram() / table->capacity);
        ds_tcam_per_entry_used    = 0;
        ds_map_ram_per_entry_used = align_to_byte(table->get_consumed_sram() / table->capacity);
        ds_xbar_used              = align_to_byte(table->get_match_xbar_consume());
        ds_logical_ids_used       = 1;
      } break;
      case DSType::Register: {
        const Register *reg       = dynamic_cast<const Register *>(ds);
        ds_sram_per_entry_used    = align_to_byte(reg->get_consumed_sram() / reg->capacity);
        ds_tcam_per_entry_used    = 0;
        ds_map_ram_per_entry_used = align_to_byte(reg->get_consumed_sram() / reg->capacity);
        ds_xbar_used              = align_to_byte(reg->index_size);
        ds_logical_ids_used       = reg->get_num_logical_ids();
      } break;
      case DSType::Meter: {
        const Meter *meter        = dynamic_cast<const Meter *>(ds);
        ds_sram_per_entry_used    = align_to_byte(meter->get_consumed_sram() / meter->capacity);
        ds_tcam_per_entry_used    = 0;
        ds_map_ram_per_entry_used = align_to_byte(meter->get_consumed_sram() / meter->capacity);
        ds_xbar_used              = align_to_byte(meter->get_match_xbar_consume());
        ds_logical_ids_used       = 1;
      } break;
      case DSType::Hash: {
        const Hash *hash          = dynamic_cast<const Hash *>(ds);
        ds_sram_per_entry_used    = 0;
        ds_tcam_per_entry_used    = 0;
        ds_map_ram_per_entry_used = 0;
        ds_xbar_used              = align_to_byte(hash->get_match_xbar_consume());
        ds_logical_ids_used       = 1;
      } break;
      case DSType::Digest: {
        ds_sram_per_entry_used    = 0;
        ds_tcam_per_entry_used    = 0;
        ds_map_ram_per_entry_used = 0;
        ds_xbar_used              = 0;
        ds_logical_ids_used       = 0;
      } break;
      case DSType::LPM: {
        const LPM *lpm            = dynamic_cast<const LPM *>(ds);
        ds_sram_per_entry_used    = 0;
        ds_tcam_per_entry_used    = align_to_byte(lpm->get_consumed_tcam() / lpm->capacity);
        ds_map_ram_per_entry_used = 0;
        ds_xbar_used              = align_to_byte(lpm->get_match_xbar_consume());
        ds_logical_ids_used       = 1;
      } break;
      default: {
        panic("Unexpected data structure type: %s", ds->id.c_str());
      }
      }

      const GRBVar &e = e_s_t.at({s, ds->id});
      const GRBVar &p = p_s_t.at({s, ds->id});

      sram_used_per_stage[s]        = sram_used_per_stage[s] + e * ds_sram_per_entry_used;
      tcam_used_per_stage[s]        = tcam_used_per_stage[s] + e * ds_tcam_per_entry_used;
      map_ram_used_per_stage[s]     = map_ram_used_per_stage[s] + e * ds_map_ram_per_entry_used;
      xbar_used_per_stage[s]        = xbar_used_per_stage[s] + p * ds_xbar_used;
      logical_ids_used_per_stage[s] = logical_ids_used_per_stage[s] + p * ds_logical_ids_used;
    }

    model.addConstr(sram_used_per_stage[s] <= TotalSRAMPerStage, "sram_used_per_stage_" + std::to_string(s));
    model.addConstr(tcam_used_per_stage[s] <= TotalTCAMPerStage, "tcam_used_per_stage_" + std::to_string(s));
    model.addConstr(map_ram_used_per_stage[s] <= TotalMapRAMPerStage, "map_ram_used_per_stage_" + std::to_string(s));
    model.addConstr(xbar_used_per_stage[s] <= TotalXbarPerStage, "xbar_used_per_stage_" + std::to_string(s));
    model.addConstr(logical_ids_used_per_stage[s] <= MaxLogicalIDs, "logical_ids_used_per_stage_" + std::to_string(s));
  }

  GRBLinExpr digests_used = 0;
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    if (ds->type == DSType::Digest) {
      digests_used = digests_used + 1;
    }
  }
  model.addConstr(digests_used <= MaxDigests, "digests_used");

  // ***************************
  // happens_before constraints
  // ***************************

  // For every table B depending on table A, B's first stage follows A's last stage.
  for (const auto &[ti, tj] : ds_relationships.happens_before) {
    const GRBVar &l_ti = l_t.at(ti);
    const GRBVar &f_tj = f_t.at(tj);
    // l_ti <= f_tj
    model.addConstr(l_ti <= f_tj, ti + "_happens_before_" + tj);
  }

  // Building f_aux and l_aux.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    GRBLinExpr f_t_aux_sum         = 0;
    GRBLinExpr f_t_aux_times_s_sum = 0;
    GRBLinExpr l_t_aux_sum         = 0;
    GRBLinExpr l_t_aux_times_s_sum = 0;

    for (int s = 0; s < TotalStages; s++) {
      const GRBVar &f_aux = f_s_t_aux.at({s, ds->id});
      const GRBVar &l_aux = l_s_t_aux.at({s, ds->id});

      f_t_aux_sum         = f_t_aux_sum + f_aux;
      f_t_aux_times_s_sum = f_t_aux_times_s_sum + s * f_aux;

      l_t_aux_sum         = l_t_aux_sum + l_aux;
      l_t_aux_times_s_sum = l_t_aux_times_s_sum + s * l_aux;
    }

    model.addConstr(f_t_aux_sum == 1, "f_t_aux_sum_" + ds->id + "_eq_1");
    model.addConstr(f_t_aux_times_s_sum == f_t.at(ds->id), "f_t_aux_times_s_sum_" + ds->id + "_eq_f_t");

    model.addConstr(l_t_aux_sum == 1, "l_t_aux_sum_" + ds->id + "_eq_1");
    model.addConstr(l_t_aux_times_s_sum == l_t.at(ds->id), "l_t_aux_times_s_sum_" + ds->id + "_eq_f_t");
  }

  // No ds entries before the first stage.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    for (int s = 0; s < TotalStages; s++) {
      GRBLinExpr lower_sum = 0;
      GRBLinExpr upper_sum = 0;

      for (int ss = 0; ss < TotalStages; ss++) {
        const GRBVar &f_aux = f_s_t_aux.at({ss, ds->id});
        const GRBVar &l_aux = l_s_t_aux.at({ss, ds->id});
        if (ss < s) {
          lower_sum = lower_sum + f_aux;
        } else if (ss > s) {
          upper_sum = upper_sum + l_aux;
        } else {
          lower_sum = lower_sum + f_aux;
          upper_sum = upper_sum + l_aux;
        }
      }

      const GRBVar &p_aux = p_s_t.at({s, ds->id});

      model.addConstr(p_aux <= lower_sum, "p_aux_le_lower_sum_" + ds->id + "_s" + std::to_string(s));
      model.addConstr(p_aux <= upper_sum, "p_aux_le_upper_sum_" + ds->id + "_s" + std::to_string(s));
    }
  }

  model.optimize();

  pipeline_resources.used_digests = static_cast<u8>(digests_used.getValue());

  for (int s = 0; s < TotalStages; s++) {
    pipeline_resources.stages[s].available_sram -= static_cast<bits_t>(sram_used_per_stage[s].getValue());
    pipeline_resources.stages[s].available_tcam -= static_cast<bits_t>(tcam_used_per_stage[s].getValue());
    pipeline_resources.stages[s].available_map_ram -= static_cast<bits_t>(map_ram_used_per_stage[s].getValue());
    pipeline_resources.stages[s].available_exact_match_xbar -= static_cast<bits_t>(xbar_used_per_stage[s].getValue());
    pipeline_resources.stages[s].available_logical_ids -= static_cast<bits_t>(logical_ids_used_per_stage[s].getValue());

    for (const DS *ds : ds_relationships.primitive_data_structures) {
      const int p = std::round(p_s_t.at({s, ds->id}).get(GRB_DoubleAttr_X));
      if (p != 0) {
        pipeline_resources.stages[s].data_structures.insert(ds->id);
      }

      // const z3::expr entries = model.eval(e_s_t.at({s, ds->id}));
      // const z3::expr f_aux   = model.eval(f_s_t_aux.at({s, ds->id}));
      // const z3::expr l_aux   = model.eval(l_s_t_aux.at({s, ds->id}));
      // std::cout << "Data Structure " << ds->id << ":\n";
      // std::cout << "  Placed stages: " << model.eval(f_t.at(ds->id)) << "-" << model.eval(l_t.at(ds->id)) << "\n";
      // std::cout << "  Stage " << s << ": " << entries << " (placed=" << placed << ", f_aux=" << f_aux << ", l_aux=" << l_aux << ")\n";
    }
  }

  return pipeline_resources;
}

#else
ActiveSolver get_active_solver() { return ActiveSolver::Z3; }

PlacementResult find_placements(const Pipeline &pipeline, const DS *target_ds, const std::unordered_set<DS_ID> &deps) {
  const ds_relationships_t ds_relationships = get_ds_relationships(pipeline, target_ds, deps);

  const int TotalStages         = pipeline.properties.stages;
  const int MaxLogicalIDs       = pipeline.properties.max_logical_sram_and_tcam_tables_per_stage;
  const int TotalXbarPerStage   = pipeline.properties.exact_match_xbar_per_stage;
  const int TotalSRAMPerStage   = pipeline.properties.sram_per_stage;
  const int TotalTCAMPerStage   = pipeline.properties.tcam_per_stage;
  const int TotalMapRAMPerStage = pipeline.properties.map_ram_per_stage;
  const int MaxDigests          = pipeline.properties.max_digests;

  z3::context ctx;
  // z3::solver solver(ctx);
  z3::optimize solver(ctx);

  std::map<std::pair<int, DS_ID>, z3::expr> p_s_t;
  std::map<std::pair<int, DS_ID>, z3::expr> e_s_t;
  std::map<DS_ID, z3::expr> f_t;
  std::map<DS_ID, z3::expr> l_t;
  std::map<std::pair<int, DS_ID>, z3::expr> f_s_t_aux;
  std::map<std::pair<int, DS_ID>, z3::expr> l_s_t_aux;

  for (int s = 0; s < TotalStages; s++) {
    for (const DS *ds : ds_relationships.primitive_data_structures) {
      std::stringstream p_ss, e_ss;

      p_ss << "p_" << s << "_" << ds;
      e_ss << "e_" << s << "_" << ds;

      z3::expr p = ctx.int_const(p_ss.str().c_str());
      z3::expr e = ctx.int_const(e_ss.str().c_str());

      p_s_t.insert({{s, ds->id}, p});
      e_s_t.insert({{s, ds->id}, e});

      solver.add(p >= 0);
      solver.add(p <= 1);

      solver.add(e >= 0);
    }
  }

  for (const DS *ds : ds_relationships.primitive_data_structures) {
    std::stringstream f_ss, l_ss;

    f_ss << "f_" << ds;
    l_ss << "l_" << ds;

    z3::expr f = ctx.int_const(f_ss.str().c_str());
    z3::expr l = ctx.int_const(l_ss.str().c_str());

    f_t.insert({ds->id, f});
    l_t.insert({ds->id, l});

    solver.add(f >= 0);
    solver.add(l <= TotalStages - 1);

    solver.add(l >= 0);
    solver.add(l <= TotalStages - 1);

    solver.add(f <= l);
  }

  for (const DS *ds : ds_relationships.primitive_data_structures) {
    for (int s = 0; s < TotalStages; s++) {
      std::stringstream f_s_t_aux_ss, l_s_t_aux_ss;

      f_s_t_aux_ss << "f_" << s << "_" << ds << "_aux";
      l_s_t_aux_ss << "l_" << s << "_" << ds << "_aux";

      z3::expr f_aux = ctx.int_const(f_s_t_aux_ss.str().c_str());
      z3::expr l_aux = ctx.int_const(l_s_t_aux_ss.str().c_str());

      f_s_t_aux.insert({{s, ds->id}, f_aux});
      l_s_t_aux.insert({{s, ds->id}, l_aux});

      solver.add(f_aux >= 0);
      solver.add(f_aux <= p_s_t.at({s, ds->id}));

      solver.add(l_aux >= 0);
      solver.add(l_aux <= p_s_t.at({s, ds->id}));
    }
  }

  // ***************************
  // Assignment constraints
  // ***************************

  // Allocating all entries for each data structure.
  // Also, allocating means placing.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    z3::expr capacity             = ctx.int_val(0);
    bool can_span_multiple_stages = false;

    switch (ds->type) {
    case DSType::Table: {
      const Table *table       = dynamic_cast<const Table *>(ds);
      capacity                 = ctx.int_val(table->capacity);
      can_span_multiple_stages = true;
    } break;
    case DSType::Register: {
      const Register *reg = dynamic_cast<const Register *>(ds);
      capacity            = ctx.int_val(reg->capacity);
    } break;
    case DSType::Meter: {
      const Meter *meter = dynamic_cast<const Meter *>(ds);
      capacity           = ctx.int_val(meter->capacity);
    } break;
    case DSType::Hash: {
      // Assume the hash takes 1 entry. This entry is meaningless.
      capacity = ctx.int_val(1);
    } break;
    case DSType::Digest: {
      // Assume the digest takes 1 entry. This entry is meaningless.
      capacity = ctx.int_val(1);
    } break;
    case DSType::LPM: {
      const LPM *lpm = dynamic_cast<const LPM *>(ds);
      capacity       = ctx.int_val(lpm->capacity);
    } break;
    default: {
      panic("Unexpected data structure type: %s", ds->id.c_str());
    }
    }

    z3::expr e_t = ctx.int_val(0);
    for (int s = 0; s < TotalStages; s++) {
      e_t = e_t + e_s_t.at({s, ds->id});
    }
    solver.add(e_t == capacity);

    for (int s = 0; s < TotalStages; s++) {
      z3::expr p = p_s_t.at({s, ds->id});
      z3::expr e = e_s_t.at({s, ds->id});
      solver.add(e <= capacity * p);
      solver.add(e >= p);
    }

    if (!can_span_multiple_stages) {
      z3::expr p_sum = ctx.int_val(0);
      for (int s = 0; s < TotalStages; s++) {
        p_sum = p_sum + p_s_t.at({s, ds->id});
      }
      solver.add(p_sum <= 1);
    }
  }

  // ***************************
  // Capacity constraints
  // ***************************

  std::vector<z3::expr> sram_used_per_stage(TotalStages, ctx.int_val(0));
  std::vector<z3::expr> tcam_used_per_stage(TotalStages, ctx.int_val(0));
  std::vector<z3::expr> map_ram_used_per_stage(TotalStages, ctx.int_val(0));
  std::vector<z3::expr> xbar_used_per_stage(TotalStages, ctx.int_val(0));
  std::vector<z3::expr> logical_ids_used_per_stage(TotalStages, ctx.int_val(0));

  for (int s = 0; s < TotalStages; s++) {
    for (const DS *ds : ds_relationships.primitive_data_structures) {
      z3::expr ds_sram_per_entry_used    = ctx.int_val(0);
      z3::expr ds_tcam_per_entry_used    = ctx.int_val(0);
      z3::expr ds_map_ram_per_entry_used = ctx.int_val(0);
      z3::expr ds_xbar_used              = ctx.int_val(0);
      z3::expr ds_logical_ids_used       = ctx.int_val(0);

      switch (ds->type) {
      case DSType::Table: {
        const Table *table        = dynamic_cast<const Table *>(ds);
        ds_sram_per_entry_used    = ctx.int_val(align_to_byte(table->get_consumed_sram() / table->capacity));
        ds_tcam_per_entry_used    = ctx.int_val(0);
        ds_map_ram_per_entry_used = ctx.int_val(align_to_byte(table->get_consumed_sram() / table->capacity));
        ds_xbar_used              = ctx.int_val(align_to_byte(table->get_match_xbar_consume()));
        ds_logical_ids_used       = ctx.int_val(1);
      } break;
      case DSType::Register: {
        const Register *reg       = dynamic_cast<const Register *>(ds);
        ds_sram_per_entry_used    = ctx.int_val(align_to_byte(reg->get_consumed_sram() / reg->capacity));
        ds_tcam_per_entry_used    = ctx.int_val(0);
        ds_map_ram_per_entry_used = ctx.int_val(align_to_byte(reg->get_consumed_sram() / reg->capacity));
        ds_xbar_used              = ctx.int_val(align_to_byte(reg->index_size));
        ds_logical_ids_used       = ctx.int_val(reg->get_num_logical_ids());
      } break;
      case DSType::Meter: {
        const Meter *meter        = dynamic_cast<const Meter *>(ds);
        ds_sram_per_entry_used    = ctx.int_val(align_to_byte(meter->get_consumed_sram() / meter->capacity));
        ds_tcam_per_entry_used    = ctx.int_val(0);
        ds_map_ram_per_entry_used = ctx.int_val(align_to_byte(meter->get_consumed_sram() / meter->capacity));
        ds_xbar_used              = ctx.int_val(align_to_byte(meter->get_match_xbar_consume()));
        ds_logical_ids_used       = ctx.int_val(1);
      } break;
      case DSType::Hash: {
        const Hash *hash          = dynamic_cast<const Hash *>(ds);
        ds_sram_per_entry_used    = ctx.int_val(0);
        ds_tcam_per_entry_used    = ctx.int_val(0);
        ds_map_ram_per_entry_used = ctx.int_val(0);
        ds_xbar_used              = ctx.int_val(align_to_byte(hash->get_match_xbar_consume()));
        ds_logical_ids_used       = ctx.int_val(1);
      } break;
      case DSType::Digest: {
        ds_sram_per_entry_used    = ctx.int_val(0);
        ds_tcam_per_entry_used    = ctx.int_val(0);
        ds_map_ram_per_entry_used = ctx.int_val(0);
        ds_xbar_used              = ctx.int_val(0);
        ds_logical_ids_used       = ctx.int_val(0);
      } break;
      case DSType::LPM: {
        const LPM *lpm            = dynamic_cast<const LPM *>(ds);
        ds_sram_per_entry_used    = ctx.int_val(0);
        ds_tcam_per_entry_used    = ctx.int_val(align_to_byte(lpm->get_consumed_tcam() / lpm->capacity));
        ds_map_ram_per_entry_used = ctx.int_val(0);
        ds_xbar_used              = ctx.int_val(align_to_byte(lpm->get_match_xbar_consume()));
        ds_logical_ids_used       = ctx.int_val(1);
      } break;
      default: {
        panic("Unexpected data structure type: %s", ds->id.c_str());
      }
      }

      z3::expr e = e_s_t.at({s, ds->id});
      z3::expr p = p_s_t.at({s, ds->id});

      sram_used_per_stage[s]        = sram_used_per_stage[s] + e * ds_sram_per_entry_used;
      tcam_used_per_stage[s]        = tcam_used_per_stage[s] + e * ds_tcam_per_entry_used;
      map_ram_used_per_stage[s]     = map_ram_used_per_stage[s] + e * ds_map_ram_per_entry_used;
      xbar_used_per_stage[s]        = xbar_used_per_stage[s] + p * ds_xbar_used;
      logical_ids_used_per_stage[s] = logical_ids_used_per_stage[s] + p * ds_logical_ids_used;
    }

    solver.add(sram_used_per_stage[s] <= TotalSRAMPerStage);
    solver.add(tcam_used_per_stage[s] <= TotalTCAMPerStage);
    solver.add(map_ram_used_per_stage[s] <= TotalMapRAMPerStage);
    solver.add(xbar_used_per_stage[s] <= TotalXbarPerStage);
    solver.add(logical_ids_used_per_stage[s] <= MaxLogicalIDs);
  }

  z3::expr digests_used = ctx.int_val(0);
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    if (ds->type == DSType::Digest) {
      digests_used = digests_used + ctx.int_val(1);
    }
  }

  solver.add(digests_used <= MaxDigests);

  // ***************************
  // happens_before constraints
  // ***************************

  // For every table B depending on table A, B's first stage follows A's last stage.
  for (const auto &[ti, tj] : ds_relationships.happens_before) {
    solver.add(l_t.at(ti) <= f_t.at(tj));
  }

  // Building f_aux and l_aux.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    z3::expr f_t_aux_sum         = ctx.int_val(0);
    z3::expr f_t_aux_times_s_sum = ctx.int_val(0);
    z3::expr l_t_aux_sum         = ctx.int_val(0);
    z3::expr l_t_aux_times_s_sum = ctx.int_val(0);

    for (int s = 0; s < TotalStages; s++) {
      f_t_aux_sum         = f_t_aux_sum + f_s_t_aux.at({s, ds->id});
      f_t_aux_times_s_sum = f_t_aux_times_s_sum + s * f_s_t_aux.at({s, ds->id});

      l_t_aux_sum         = l_t_aux_sum + l_s_t_aux.at({s, ds->id});
      l_t_aux_times_s_sum = l_t_aux_times_s_sum + s * l_s_t_aux.at({s, ds->id});
    }

    solver.add(f_t_aux_sum == 1);
    solver.add(f_t_aux_times_s_sum == f_t.at(ds->id));

    solver.add(l_t_aux_sum == 1);
    solver.add(l_t_aux_times_s_sum == l_t.at(ds->id));
  }

  // No ds entries before the first stage.
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    for (int s = 0; s < TotalStages; s++) {
      z3::expr lower_sum = ctx.int_val(0);
      z3::expr upper_sum = ctx.int_val(0);

      for (int ss = 0; ss < TotalStages; ss++) {
        z3::expr f_aux = f_s_t_aux.at({ss, ds->id});
        z3::expr l_aux = l_s_t_aux.at({ss, ds->id});
        if (ss < s) {
          lower_sum = lower_sum + f_aux;
        } else if (ss > s) {
          upper_sum = upper_sum + l_aux;
        } else {
          lower_sum = lower_sum + f_aux;
          upper_sum = upper_sum + l_aux;
        }
      }

      z3::expr p_aux = p_s_t.at({s, ds->id});

      solver.add(p_aux <= lower_sum);
      solver.add(p_aux <= upper_sum);
    }
  }

  // Minimize the last stage of the data structures.
  z3::expr last_stage_sum = ctx.int_val(0);
  for (const DS *ds : ds_relationships.primitive_data_structures) {
    last_stage_sum = last_stage_sum + l_t.at(ds->id);
  }
  solver.minimize(last_stage_sum);

  const z3::check_result solver_result = solver.check();
  if (solver_result == z3::unsat) {
    return PlacementStatus::Unsat;
  } else if (solver_result == z3::unknown) {
    return PlacementStatus::Unknown;
  }

  const z3::model model = solver.get_model();

  PipelineResources pipeline_resources(pipeline.properties);
  pipeline_resources.used_digests = model.eval(digests_used).get_numeral_int();

  for (int s = 0; s < TotalStages; s++) {
    pipeline_resources.stages[s].available_sram -= model.eval(sram_used_per_stage[s]).get_numeral_int();
    pipeline_resources.stages[s].available_tcam -= model.eval(tcam_used_per_stage[s]).get_numeral_int();
    pipeline_resources.stages[s].available_map_ram -= model.eval(map_ram_used_per_stage[s]).get_numeral_int();
    pipeline_resources.stages[s].available_exact_match_xbar -= model.eval(xbar_used_per_stage[s]).get_numeral_int();
    pipeline_resources.stages[s].available_logical_ids -= model.eval(logical_ids_used_per_stage[s]).get_numeral_int();

    for (const DS *ds : ds_relationships.primitive_data_structures) {
      const z3::expr placed = model.eval(p_s_t.at({s, ds->id}));
      if (placed.get_numeral_int()) {
        pipeline_resources.stages[s].data_structures.insert(ds->id);
      }

      // const z3::expr entries = model.eval(e_s_t.at({s, ds->id}));
      // const z3::expr f_aux   = model.eval(f_s_t_aux.at({s, ds->id}));
      // const z3::expr l_aux   = model.eval(l_s_t_aux.at({s, ds->id}));
      // std::cout << "Data Structure " << ds->id << ":\n";
      // std::cout << "  Placed stages: " << model.eval(f_t.at(ds->id)) << "-" << model.eval(l_t.at(ds->id)) << "\n";
      // std::cout << "  Stage " << s << ": " << entries << " (placed=" << placed << ", f_aux=" << f_aux << ", l_aux=" << l_aux << ")\n";
    }
  }

  return pipeline_resources;
}

#endif

} // namespace SolverPlacer
} // namespace Tofino
} // namespace LibSynapse