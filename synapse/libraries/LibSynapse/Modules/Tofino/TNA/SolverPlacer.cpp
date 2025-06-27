#include <LibSynapse/Modules/Tofino/TNA/SolverPlacer.h>

#include <z3++.h>

namespace LibSynapse {
namespace Tofino {
namespace SolverPlacer {

PlacementResult find_placements(const Pipeline &pipeline, const DS *target_ds, const std::unordered_set<DS_ID> &deps) {
  const int S      = pipeline.properties.stages;
  const int L      = pipeline.properties.max_logical_sram_and_tcam_tables_per_stage;
  const int Xbar   = pipeline.properties.exact_match_xbar_per_stage;
  const int SRAM   = pipeline.properties.sram_per_stage;
  const int TCAM   = pipeline.properties.tcam_per_stage;
  const int MapRAM = pipeline.properties.map_ram_per_stage;

  z3::context ctx;
  z3::solver solver(ctx);

  std::vector<PlacementRequest> placement_requests = pipeline.placement_requests;
  if (!pipeline.already_requested(target_ds->id)) {
    placement_requests.push_back({target_ds->id, deps});
  }

  std::unordered_set<const DS *> primitive_data_structures;
  std::vector<std::pair<DS_ID, DS_ID>> dependencies;

  for (const PlacementRequest &request : placement_requests) {
    const DS *requested_ds = pipeline.data_structures.get_ds_from_id(request.ds);
    if (requested_ds->primitive) {
      primitive_data_structures.insert(requested_ds);
      continue;
    }

    std::unordered_set<DS_ID> cummulative_deps;
    for (DS_ID dep_id : request.deps) {
      const DS *dep = pipeline.data_structures.get_ds_from_id(dep_id);
      if (dep->primitive) {
        cummulative_deps.insert(dep_id);
        continue;
      }

      for (const std::unordered_set<DS_ID> &data_structures : dep->get_internal_primitive_ids()) {
        cummulative_deps.insert(data_structures.begin(), data_structures.end());
      }
    }

    for (const std::unordered_set<const DS *> &data_structures : requested_ds->get_internal_primitive()) {
      for (const DS *ds : data_structures) {
        for (DS_ID dep : cummulative_deps) {
          dependencies.emplace_back(ds->id, dep);
        }
      }

      primitive_data_structures.insert(data_structures.begin(), data_structures.end());

      for (const DS *ds : data_structures) {
        cummulative_deps.insert(ds->id);
      }
    }
  }

  std::cerr << "=====================================\n";
  std::cerr << " SOLVER PLACER\n";
  std::cerr << "=====================================\n";

  std::cerr << "Placing data structure " << target_ds->id << " with dependencies: ";
  for (const DS_ID &dep : deps) {
    std::cerr << dep << " ";
  }
  std::cerr << "\n";

  std::cerr << "Primitive data structures: ";
  for (const DS *ds : primitive_data_structures) {
    std::cerr << ds->id << " ";
  }
  std::cerr << "\n";
  std::cerr << "Dependencies: ";
  for (const auto &[ti, tj] : dependencies) {
    std::cerr << ti << " -> " << tj << " ";
  }
  std::cerr << "\n";

  std::map<std::pair<int, DS_ID>, z3::expr> p_s_t;
  std::map<std::pair<int, DS_ID>, z3::expr> e_s_t;
  std::map<DS_ID, z3::expr> f_t;
  std::map<DS_ID, z3::expr> l_t;
  std::map<std::pair<int, DS_ID>, z3::expr> f_s_t_aux;
  std::map<std::pair<int, DS_ID>, z3::expr> l_s_t_aux;

  for (int s = 0; s < S; s++) {
    for (const DS *ds : primitive_data_structures) {
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

  for (const DS *ds : primitive_data_structures) {
    std::stringstream f_ss, l_ss;

    f_ss << "f_" << ds;
    l_ss << "l_" << ds;

    z3::expr f = ctx.int_const(f_ss.str().c_str());
    z3::expr l = ctx.int_const(l_ss.str().c_str());

    f_t.insert({ds->id, f});
    l_t.insert({ds->id, l});

    solver.add(f >= 0);
    solver.add(l <= S - 1);

    solver.add(l >= 0);
    solver.add(l <= S - 1);

    solver.add(f <= l);
  }

  for (const DS *ds : primitive_data_structures) {
    for (int s = 0; s < S; s++) {
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
  for (const DS *ds : primitive_data_structures) {
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
    for (int s = 0; s < S; s++) {
      e_t = e_t + e_s_t.at({s, ds->id});
    }
    solver.add(e_t == capacity);

    for (int s = 0; s < S; s++) {
      z3::expr p = p_s_t.at({s, ds->id});
      z3::expr e = e_s_t.at({s, ds->id});
      solver.add(e <= capacity * p);
      solver.add(e >= p);
    }

    if (!can_span_multiple_stages) {
      z3::expr p_sum = ctx.int_val(0);
      for (int s = 0; s < S; s++) {
        p_sum = p_sum + p_s_t.at({s, ds->id});
      }
      solver.add(p_sum <= 1);
    }
  }

  // ***************************
  // Capacity constraints
  // ***************************

  for (int s = 0; s < S; s++) {
    z3::expr sram_used        = ctx.int_val(0);
    z3::expr tcam_used        = ctx.int_val(0);
    z3::expr map_ram_used     = ctx.int_val(0);
    z3::expr xbar_used        = ctx.int_val(0);
    z3::expr logical_ids_used = ctx.int_val(0);

    for (const DS *ds : primitive_data_structures) {
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

      sram_used        = sram_used + e * ds_sram_per_entry_used;
      map_ram_used     = map_ram_used + e * ds_map_ram_per_entry_used;
      xbar_used        = xbar_used + p * ds_xbar_used;
      logical_ids_used = logical_ids_used + p;
    }

    solver.add(sram_used <= SRAM);
    solver.add(tcam_used <= TCAM);
    solver.add(map_ram_used <= MapRAM);
    solver.add(xbar_used <= Xbar);
    solver.add(logical_ids_used <= L);
  }

  // ***************************
  // Dependencies constraints
  // ***************************

  // For every table B depending on table A, B's first stage follows A's last stage.
  for (const auto &[ti, tj] : dependencies) {
    solver.add(l_t.at(ti) <= f_t.at(tj));
  }

  // Building f_aux and l_aux.
  for (const DS *ds : primitive_data_structures) {
    z3::expr f_t_aux_sum         = ctx.int_val(0);
    z3::expr f_t_aux_times_s_sum = ctx.int_val(0);
    z3::expr l_t_aux_sum         = ctx.int_val(0);
    z3::expr l_t_aux_times_s_sum = ctx.int_val(0);

    for (int s = 0; s < S; s++) {
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
  for (const DS *ds : primitive_data_structures) {
    z3::expr f = f_t.at(ds->id);
    z3::expr l = l_t.at(ds->id);

    for (int s = 0; s < S; s++) {
      z3::expr p = p_s_t.at({s, ds->id});

      z3::expr lower_sum = ctx.int_val(0);
      z3::expr upper_sum = ctx.int_val(0);

      for (int ss = 0; ss < S; ss++) {
        if (ss < s) {
          lower_sum = lower_sum + f_s_t_aux.at({ss, ds->id});
        } else if (ss > s) {
          upper_sum = upper_sum + l_s_t_aux.at({ss, ds->id});
        } else {
          lower_sum = lower_sum + f_s_t_aux.at({ss, ds->id});
          upper_sum = upper_sum + l_s_t_aux.at({ss, ds->id});
        }
      }

      z3::expr p_aux = p_s_t.at({s, ds->id});

      solver.add(p_aux <= lower_sum);
      solver.add(p_aux <= upper_sum);
    }
  }

  // std::cout << "Solver:\n";
  // std::cout << solver << "\n";

  const z3::check_result result = solver.check();

  if (result == z3::unsat) {
    std::cout << "UNSAT\n";
    return PlacementStatus::Unknown;
  } else if (result == z3::unknown) {
    std::cout << "UNKNOWN\n";
    return PlacementStatus::Unknown;
  }

  const z3::model model = solver.get_model();

  for (const DS *ds : primitive_data_structures) {
    std::cout << "Data Structure " << ds->id << ":\n";
    std::cout << "  First stage: " << model.eval(f_t.at(ds->id)) << "\n";
    std::cout << "  Last stage:  " << model.eval(l_t.at(ds->id)) << "\n";
    for (int s = 0; s < S; s++) {
      const z3::expr entries = model.eval(e_s_t.at({s, ds->id}));
      const z3::expr placed  = model.eval(p_s_t.at({s, ds->id}));
      const z3::expr f_aux   = model.eval(f_s_t_aux.at({s, ds->id}));
      const z3::expr l_aux   = model.eval(l_s_t_aux.at({s, ds->id}));

      std::cout << "  Stage " << s << ": " << entries << " (placed=" << placed << ", f_aux=" << f_aux << ", l_aux=" << l_aux << ")\n";
    }
  }

  return PlacementStatus::Unknown;
}

} // namespace SolverPlacer
} // namespace Tofino
} // namespace LibSynapse