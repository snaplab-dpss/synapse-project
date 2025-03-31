#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/TNA/SimplerPlacer.h>
#include <LibSynapse/Modules/Tofino/TNA/Parser.h>
#include <LibCore/Types.h>

#include <unordered_map>

namespace LibSynapse {
namespace Tofino {

struct tna_properties_t {
  int total_ports;
  int total_recirc_ports;
  bytes_t max_packet_bytes_in_condition;
  int pipes;
  int stages;
  bits_t sram_per_stage;
  bits_t tcam_per_stage;
  bits_t map_ram_per_stage;
  int max_logical_tcam_tables_per_stage;
  int max_logical_sram_and_tcam_tables_per_stage;
  bits_t phv_size;
  int phv_8bit_containers;
  int phv_16bit_containers;
  int phv_32bit_containers;
  bits_t packet_buffer_size;
  bits_t exact_match_xbar_per_stage;
  int max_exact_match_keys;
  bits_t ternary_match_xbar;
  int max_ternary_match_keys;
  bits_t max_salu_size;
  u8 max_digests;
};

struct tofino_port_t {
  u16 nf_device;
  u16 front_panel_port;
  bps_t capacity;
};

struct tofino_recirculation_port_t {
  u16 dev_port;
  bps_t capacity;
};

struct tna_config_t {
  tna_properties_t properties;
  std::vector<tofino_port_t> ports;
  std::vector<tofino_recirculation_port_t> recirculation_ports;
};

class TNA {
private:
  const tna_config_t tna_config;
  SimplePlacer simple_placer;

public:
  Parser parser;

  TNA(const tna_config_t &config);
  TNA(const TNA &other);

  const tna_config_t &get_tna_config() const { return tna_config; }
  const SimplePlacer &get_simple_placer() const { return simple_placer; }

  // Tofino compiler complains if we access more than 4 bytes of the packet on
  // the same if statement.
  bool condition_meets_phv_limit(klee::ref<klee::Expr> expr) const;

  void place(const DS *ds, const std::unordered_set<DS_ID> &deps);
  PlacementStatus can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  PlacementStatus can_place_many(const std::vector<std::unordered_set<DS *>> &ds, const std::unordered_set<DS_ID> &deps) const;

  void debug() const;
};

} // namespace Tofino
} // namespace LibSynapse