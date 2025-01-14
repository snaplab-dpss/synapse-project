#include "tna.hpp"

#include "../../../util/exprs.hpp"

namespace synapse {
namespace tofino {
namespace {
TNAProperties properties_from_config(const toml::table &config) {
  assert(config.contains("switch") && "Switch configuration not found");
  assert(config["switch"].as_table()->contains("arch") && "Arch configuration not found");
  return {
      .total_ports                       = static_cast<int>(config["switch"]["front_panel_ports"].as_array()->size()),
      .total_recirc_ports                = static_cast<int>(config["switch"]["recirculation_ports"].as_array()->size()),
      .max_packet_bytes_in_condition     = *config["switch"]["arch"]["max_packet_bytes_in_condition"].value<int>(),
      .pipes                             = *config["switch"]["arch"]["pipes"].value<int>(),
      .stages                            = *config["switch"]["arch"]["stages"].value<int>(),
      .sram_per_stage                    = *config["switch"]["arch"]["sram_per_stage"].value<bits_t>(),
      .tcam_per_stage                    = *config["switch"]["arch"]["tcam_per_stage"].value<bits_t>(),
      .map_ram_per_stage                 = *config["switch"]["arch"]["map_ram_per_stage"].value<bits_t>(),
      .max_logical_tcam_tables_per_stage = *config["switch"]["arch"]["max_logical_tcam_tables_per_stage"].value<int>(),
      .max_logical_sram_and_tcam_tables_per_stage =
          *config["switch"]["arch"]["max_logical_sram_and_tcam_tables_per_stage"].value<int>(),
      .phv_size                   = *config["switch"]["arch"]["phv_size"].value<bits_t>(),
      .phv_8bit_containers        = *config["switch"]["arch"]["phv_8bit_containers"].value<int>(),
      .phv_16bit_containers       = *config["switch"]["arch"]["phv_16bit_containers"].value<int>(),
      .phv_32bit_containers       = *config["switch"]["arch"]["phv_32bit_containers"].value<int>(),
      .packet_buffer_size         = *config["switch"]["arch"]["packet_buffer_size"].value<bits_t>(),
      .exact_match_xbar_per_stage = *config["switch"]["arch"]["exact_match_xbar_per_stage"].value<bits_t>(),
      .max_exact_match_keys       = *config["switch"]["arch"]["max_exact_match_keys"].value<int>(),
      .ternary_match_xbar         = *config["switch"]["arch"]["ternary_match_xbar"].value<bits_t>(),
      .max_ternary_match_keys     = *config["switch"]["arch"]["max_ternary_match_keys"].value<int>(),
      .max_salu_size              = *config["switch"]["arch"]["max_salu_size"].value<bits_t>(),
  };
}
} // namespace

TNA::TNA(const toml::table &config) : properties(properties_from_config(config)), simple_placer(&properties) {}

TNA::TNA(const TNA &other) : properties(other.properties), simple_placer(other.simple_placer), parser(other.parser) {}

bool TNA::condition_meets_phv_limit(klee::ref<klee::Expr> expr) const {
  int total_packet_bytes_read = 0;
  symbolic_reads_t bytes_read = get_unique_symbolic_reads(expr);
  for (const symbolic_read_t &byte_read : bytes_read) {
    if (byte_read.symbol == "packet_chunks") {
      total_packet_bytes_read += 1;
    }
  }
  return total_packet_bytes_read <= properties.max_packet_bytes_in_condition;
}

void TNA::place(const DS *ds, const std::unordered_set<DS_ID> &deps) { simple_placer.place(ds, deps); }

PlacementStatus TNA::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const {
  return simple_placer.can_place(ds, deps);
}

PlacementStatus TNA::can_place_many(const std::vector<std::unordered_set<DS *>> &candidates,
                                    const std::unordered_set<DS_ID> &_deps) const {
  SimplePlacer snapshot(simple_placer);
  std::unordered_set<DS_ID> deps = _deps;

  for (const std::unordered_set<DS *> &indep_ds : candidates) {
    std::unordered_set<DS_ID> new_deps;

    for (const DS *ds : indep_ds) {
      PlacementStatus status = snapshot.can_place(ds, deps);

      if (status != PlacementStatus::SUCCESS) {
        return status;
      }

      snapshot.place(ds, deps);
      new_deps.insert(ds->id);
    }

    deps.insert(new_deps.begin(), new_deps.end());
  }

  return PlacementStatus::SUCCESS;
}

void TNA::debug() const { simple_placer.debug(); }

} // namespace tofino
} // namespace synapse