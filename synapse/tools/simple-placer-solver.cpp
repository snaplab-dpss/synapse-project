#include <LibSynapse/Modules/Tofino/TNA/SimplerPlacer.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibSynapse/Target.h>

#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include "z3++.h"

using LibSynapse::targets_config_t;
using LibSynapse::Tofino::DS_ID;
using LibSynapse::Tofino::Table;
using LibSynapse::Tofino::tna_properties_t;

int main(int argc, char **argv) {
  CLI::App app{"simple-placer-solver"};

  std::filesystem::path targets_config_file;

  app.add_option("--config", targets_config_file, "Configuration file.")->required();

  CLI11_PARSE(app, argc, argv);

  const targets_config_t targets_config(targets_config_file);
  const tna_properties_t &tna_properties = targets_config.tofino_config.properties;

  const Table t0("t0", 65536, {32, 32, 16, 16}, {32});
  const Table t1("t1", 65536, {32, 32, 16, 16}, {32});
  const Table t2("t2", 65536, {32, 32, 16, 16}, {32});

  const std::vector<Table> tables{
      t0,
      t1,
      t2,
  };
  const std::vector<std::pair<DS_ID, DS_ID>> dependencies{
      {t0.id, t1.id},
      {t0.id, t2.id},
  };

  const int S      = tna_properties.stages;
  const int L      = tna_properties.max_logical_sram_and_tcam_tables_per_stage;
  const int Xbar   = tna_properties.exact_match_xbar_per_stage;
  const int SRAM   = tna_properties.sram_per_stage;
  const int TCAM   = tna_properties.tcam_per_stage;
  const int MapRAM = tna_properties.map_ram_per_stage;

  z3::context ctx;
  z3::solver solver(ctx);

  std::map<std::pair<int, DS_ID>, z3::expr> p_s_t;
  std::map<std::pair<int, DS_ID>, z3::expr> e_s_t;
  std::map<DS_ID, z3::expr> f_t;
  std::map<DS_ID, z3::expr> l_t;
  std::map<std::pair<int, DS_ID>, z3::expr> f_s_t_aux;
  std::map<std::pair<int, DS_ID>, z3::expr> l_s_t_aux;

  for (int s = 0; s < S; s++) {
    for (const Table &table : tables) {
      std::stringstream p_ss, e_ss;

      p_ss << "p_" << s << "_" << table.id;
      e_ss << "e_" << s << "_" << table.id;

      z3::expr p = ctx.int_const(p_ss.str().c_str());
      z3::expr e = ctx.int_const(e_ss.str().c_str());

      p_s_t.insert({{s, table.id}, p});
      e_s_t.insert({{s, table.id}, e});

      solver.add(p >= 0);
      solver.add(p <= 1);

      solver.add(e >= 0);
    }
  }

  for (const Table &table : tables) {
    std::stringstream f_ss, l_ss;

    f_ss << "f_" << table.id;
    l_ss << "l_" << table.id;

    z3::expr f = ctx.int_const(f_ss.str().c_str());
    z3::expr l = ctx.int_const(l_ss.str().c_str());

    f_t.insert({table.id, f});
    l_t.insert({table.id, l});

    solver.add(f >= 0);
    solver.add(l <= S - 1);

    solver.add(l >= 0);
    solver.add(l <= S - 1);

    solver.add(f <= l);
  }

  for (const Table &table : tables) {
    for (int s = 0; s < S; s++) {
      std::stringstream f_s_t_aux_ss, l_s_t_aux_ss;

      f_s_t_aux_ss << "f_" << s << "_" << table.id << "_aux";
      l_s_t_aux_ss << "l_" << s << "_" << table.id << "_aux";

      z3::expr f_aux = ctx.int_const(f_s_t_aux_ss.str().c_str());
      z3::expr l_aux = ctx.int_const(l_s_t_aux_ss.str().c_str());

      f_s_t_aux.insert({{s, table.id}, f_aux});
      l_s_t_aux.insert({{s, table.id}, l_aux});

      solver.add(f_aux >= 0);
      solver.add(f_aux <= 1);

      solver.add(l_aux >= 0);
      solver.add(l_aux <= 1);
    }
  }

  // ***************************
  // Assignment constraints
  // ***************************

  // Allocating at least every entry of every table.
  for (const Table &table : tables) {
    z3::expr e_t = ctx.int_val(0);
    for (int s = 0; s < S; s++) {
      e_t = e_t + e_s_t.at({s, table.id});
    }
    z3::expr capacity = ctx.int_val(table.capacity);
    solver.add(e_t >= capacity);
  }

  // Allocating entries for table means to place it.
  for (int s = 0; s < S; s++) {
    for (const Table &table : tables) {
      z3::expr p        = p_s_t.at({s, table.id});
      z3::expr e        = e_s_t.at({s, table.id});
      z3::expr capacity = ctx.int_val(table.capacity);

      solver.add(e <= capacity * p);
      solver.add(e >= p);
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

    for (const Table &table : tables) {
      z3::expr table_sram_used    = ctx.int_val(table.get_consumed_sram_per_entry());
      z3::expr table_map_ram_used = ctx.int_val(table.get_consumed_sram_per_entry());
      z3::expr table_xbar_used    = ctx.int_val(table.get_match_xbar_consume());

      z3::expr e = e_s_t.at({s, table.id});
      z3::expr p = p_s_t.at({s, table.id});

      sram_used        = sram_used + e * table_sram_used;
      map_ram_used     = map_ram_used + e * table_map_ram_used;
      xbar_used        = xbar_used + p * table_xbar_used;
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
  for (const Table &table : tables) {
    z3::expr f_t_aux_sum         = ctx.int_val(0);
    z3::expr f_t_aux_times_s_sum = ctx.int_val(0);
    z3::expr l_t_aux_sum         = ctx.int_val(0);
    z3::expr l_t_aux_times_s_sum = ctx.int_val(0);

    for (int s = 0; s < S; s++) {
      f_t_aux_sum         = f_t_aux_sum + f_s_t_aux.at({s, table.id});
      f_t_aux_times_s_sum = f_t_aux_times_s_sum + s * f_s_t_aux.at({s, table.id});

      l_t_aux_sum         = l_t_aux_sum + l_s_t_aux.at({s, table.id});
      l_t_aux_times_s_sum = l_t_aux_times_s_sum + s * l_s_t_aux.at({s, table.id});
    }

    solver.add(f_t_aux_sum == 1);
    solver.add(f_t_aux_times_s_sum == f_t.at(table.id));

    solver.add(l_t_aux_sum == 1);
    solver.add(l_t_aux_times_s_sum == l_t.at(table.id));
  }

  // No table entries before the first stage.
  for (const Table &table : tables) {
    z3::expr f = f_t.at(table.id);
    z3::expr l = l_t.at(table.id);

    for (int s = 0; s < S; s++) {
      z3::expr p = p_s_t.at({s, table.id});

      z3::expr lower_sum = ctx.int_val(0);
      z3::expr upper_sum = ctx.int_val(0);

      for (int ss = 0; ss < S; ss++) {
        if (ss < s) {
          lower_sum = lower_sum + f_s_t_aux.at({ss, table.id});
        } else if (ss > s) {
          upper_sum = upper_sum + l_s_t_aux.at({ss, table.id});
        } else {
          lower_sum = lower_sum + f_s_t_aux.at({ss, table.id});
          upper_sum = upper_sum + l_s_t_aux.at({ss, table.id});
        }
      }

      z3::expr p_aux = p_s_t.at({s, table.id});

      solver.add(p_aux <= lower_sum);
      solver.add(p_aux <= upper_sum);
    }
  }

  // std::cout << "Solver:\n";
  // std::cout << solver << "\n";

  const z3::check_result result = solver.check();

  if (result == z3::unsat) {
    std::cout << "UNSAT\n";
    return 0;
  } else if (result == z3::unknown) {
    std::cout << "UNKNOWN\n";
    return 0;
  }

  const z3::model model = solver.get_model();

  // std::cout << "SAT model:\n";
  // std::cout << model << "\n";
  // for (unsigned i = 0; i < model.size(); i++) {
  //   z3::func_decl v = model[i];
  //   // this problem contains only constants
  //   assert(v.arity() == 0);
  //   std::cout << v.name() << " = " << model.get_const_interp(v) << "\n";
  // }

  for (const Table &table : tables) {
    std::cout << "Table " << table.id << ":\n";
    std::cout << "  First stage: " << model.eval(f_t.at(table.id)) << "\n";
    std::cout << "  Last stage:  " << model.eval(l_t.at(table.id)) << "\n";
    for (int s = 0; s < S; s++) {
      std::cout << "  Stage " << s << ":\n";
      std::cout << "    Placed: " << model.eval(p_s_t.at({s, table.id})) << "\n";
      std::cout << "    Entries: " << model.eval(e_s_t.at({s, table.id})) << "\n";
      std::cout << "    f_aux: " << model.eval(f_s_t_aux.at({s, table.id})) << "\n";
      std::cout << "    l_aux: " << model.eval(l_s_t_aux.at({s, table.id})) << "\n";
    }
  }

  return 0;
}